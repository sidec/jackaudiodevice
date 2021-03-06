/*
 * libjacknative.so
 *
 * Native implementation of class com.noisepages.nettoyeur.jack.JackNativeClient.
 *
 * Author:      Jens Gulden
 * Modified by: Peter J. Salomonsen
 * Modified by: Andrew Cooke
 * Modified by: Peter Brinkmann
 *
 * 2006-12-17 (PJS): Changed INF pointer to long (64bit) for amd64 support
 * 2007-04-09 (PJS): No longer object allocation for each process call
 * 2007-07-10 (AC):  - The size of the sample is found using size() rather than being
 *                     hardcoded as 4
 *                   - Some ints are cast to jsize which is long on 64 bit systems (see
 *                     ftp://www6.software.ibm.com/software/developer/jdk/64bitporting/64BitJavaPortingGuide.pdf)
 *                   - The allocation of inf->byteBufferArray[mode] is made outside the loop
 *                     over ports. This avoids a null pointer exception in Java when there are
 *                     zero ports (eg for output on a "consumer only" process).
 * 2009-03-16 (PB):  - Added support for autoconnect by name to other clients
 * 2009-05-11 (PB):  - General refactoring
 * 2009-05-11 (PB):  - Eliminated static field lookup; all values are
 * now passed as function arguments
 * 2009-05-12 (PB):  - Moved native bridge to JJackNativeClient
 * 2009-05-13 (PB):  - Got rid of some static calls
 * 2009-05-13 (PB):  - added shutdown callback
 * 2009-05-19 (PB):  - support for further streamlining of Java callback
 * 2009-07-23 (PB):  - support for arbitrary (dis)connection of ports
 * 2011-04-03 (PB):  - extracted and simplified native parts
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <jni.h>
#include <jack.h>
#include "com_noisepages_nettoyeur_jack_JackNativeClient.h"

/*
 * Maximum number of ports to be allocated. 
 */
#define MAX_PORTS 64

/*
 * Symbols for input, output. 
 */
typedef enum {
    INPUT, 
    OUTPUT, 
    MODES   /* total count */
} MODE;

/*
 * Constant strings. Some of these reflect the names of Java identifiers. 
 */
const char *CLASS_BYTEBUFFER = "java/nio/ByteBuffer";
const char *CLASS_JACKEXCEPTION = "com/noisepages/nettoyeur/jack/JackException";
const char *METHOD_PROCESS = "processBytes";
const char *METHOD_PROCESS_SIG = "([Ljava/nio/ByteBuffer;[Ljava/nio/ByteBuffer;Z)V";
const char *METHOD_SHUTDOWN = "handleShutdown";
const char *METHOD_SHUTDOWN_SIG = "()V";
const char *MODE_LABEL[MODES] = {"input", "output"};
const unsigned long MODE_JACK[MODES] = { JackPortIsInput, JackPortIsOutput };

/*
 * Global memory to store values needed for performing the Java callback
 * from the JACK thread. A pointer to this structure is passed to the 
 * process()-function as argument 'void *arg'.
*/
typedef struct Inf {
    jobject owner;
    jack_client_t *client;
    int portCount[MODES];
    jack_port_t *port[MODES][MAX_PORTS];
    jack_default_audio_sample_t *sampleBuffers[MODES][MAX_PORTS];
    jobjectArray byteBufferArray[MODES];
    int isDaemon;
} *INF;

JavaVM *cached_jvm;              /* jvm pointer */
jclass cls_ByteBuffer;    /* handle of class java.nio.ByteBuffer */
jmethodID processCallback = NULL;
jmethodID shutdownCallback = NULL;


/* 
 * Main JACK audio process chain callback. From here, we will branch into the
 * Java virtual machine to let Java code perform the processing.
 */
int process(jack_nframes_t nframes, void *arg) {
  INF inf = arg;
  JNIEnv *env;
  int mode, i;
  jboolean reallocated = JNI_FALSE;

  int res = (inf->isDaemon ?
        (*cached_jvm)->AttachCurrentThreadAsDaemon(cached_jvm,
                (void**) &env, NULL) :
        (*cached_jvm)->AttachCurrentThread(cached_jvm, (void**) &env, NULL));
  if (res) {
    fprintf(stderr, "FATAL: cannot attach JACK thread to JVM\n");
    return -1;
  }
  
  for(mode=INPUT; mode<=OUTPUT; mode++) {
    for(i=0; i<inf->portCount[mode]; i++) {
      // Only reallocate if the buffer position changes
      jack_default_audio_sample_t *tempSampleBuffer = 
            (jack_default_audio_sample_t *)
                  jack_port_get_buffer(inf->port[mode][i], nframes);
      if (tempSampleBuffer!=inf->sampleBuffers[mode][i]) {
        reallocated = JNI_TRUE;
        inf->sampleBuffers[mode][i] = tempSampleBuffer;
        jobject byteBuffer = (*env)->NewDirectByteBuffer(env, tempSampleBuffer,
                    (jsize)(nframes*sizeof(jack_default_audio_sample_t)));
        (*env)->SetObjectArrayElement(env, inf->byteBufferArray[mode],
                  (jsize) i, byteBuffer);
      }
    }
  }

  (*env)->CallVoidMethod(env, inf->owner, processCallback,
      inf->byteBufferArray[INPUT], inf->byteBufferArray[OUTPUT], reallocated);
  
  return 0;      
}

/*
 * Shutdown callback to let Java know if the client gets zombified.
 */
void shutdown(void *arg) {
  INF inf = (INF) arg;
  JNIEnv *env;

  if ((*cached_jvm)->AttachCurrentThread(cached_jvm, (void**) &env, NULL)) {
    return;
  }

  (*env)->CallVoidMethod(env, inf->owner, shutdownCallback);

  (*cached_jvm)->DetachCurrentThread(cached_jvm);
}

/*
 * Throw a JackException in Java with an optional second description text.
 */
void throwExc(JNIEnv *env, char *msg) {	
    jclass clsExc = (*env)->FindClass(env, CLASS_JACKEXCEPTION);
    char m[255] = "";
    if (msg != NULL) {
        strcat(m, msg);
    }
    if (clsExc == 0) {
        fprintf(stderr, "fatal: cannot access class JackException.\nerror:\n%s\n", m);
    } else {
        (*env)->ThrowNew(env, clsExc, m);
    }
}	

/*
 * Allocate string memory. 
 */
const char *allocchars(JNIEnv *env, jstring js) {
    return (*env)->GetStringUTFChars(env, js, 0);
}

/*
 * Deallocate string memory.
 */
void freechars(JNIEnv *env, jstring js, const char *s) {
    (*env)->ReleaseStringUTFChars(env, js, s);
}

/*
 * Get env pointer for current thread.
 */
int getEnv(JNIEnv **envPtr) {
  return (*cached_jvm)->GetEnv(cached_jvm, (void**) envPtr, JNI_VERSION_1_4);
}

/*
 * Init function, implicitly called by JVM.
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
  JNIEnv *env;

  cached_jvm = jvm;

  if (getEnv(&env)) {
    return JNI_ERR;
  }

  jobject obj = (*env)->FindClass(env, CLASS_BYTEBUFFER);	
  if (obj == NULL) {
    return JNI_ERR;
  }
  cls_ByteBuffer = (*env)->NewWeakGlobalRef(env, obj);	

  return JNI_VERSION_1_4;
}

/*
 * Shutdown function, implicitly called by JVM.
 */
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *jvm, void *reserved) {
  JNIEnv *env;
  if (getEnv(&env)) {
    return;
  }
  (*env)->DeleteWeakGlobalRef(env, cls_ByteBuffer);
}

void closeClient(JNIEnv *env, jobject obj, INF inf) {
  if (inf!=0) {
    if (inf->client!=NULL) {
      jack_client_close(inf->client);
      if (inf->byteBufferArray[INPUT]!=NULL) {
        (*env)->DeleteGlobalRef(env, inf->byteBufferArray[INPUT]);
        (*env)->DeleteGlobalRef(env, inf->byteBufferArray[OUTPUT]);
      }
    }
    (*env)->DeleteWeakGlobalRef(env, inf->owner);
    free(inf);
  }
}

/*
 * private native long openClient(String, int, int) throws JackException
 */
JNIEXPORT jlong JNICALL Java_com_noisepages_nettoyeur_jack_JackNativeClient_openClient(JNIEnv *env, jobject obj, jstring clientName, jint portsIn, jint portsOut, jboolean isDaemon) {
  if (processCallback==NULL) { // first call?
    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID mid = (*env)->GetMethodID(env, cls,
              METHOD_PROCESS, METHOD_PROCESS_SIG);
    if (mid==NULL) {
      throwExc(env, "process method not found");
      return 0;
    }
    processCallback = mid;  // no global ref necessary

    mid = (*env)->GetMethodID(env, cls,
              METHOD_SHUTDOWN, METHOD_SHUTDOWN_SIG);
    if (mid==NULL) {
      throwExc(env, "shutdown method not found");
      return 0;
    }
    shutdownCallback = mid;  // no global ref necessary
  }

  INF inf = (INF) malloc(sizeof(struct Inf));
  if (inf==NULL) {
    throwExc(env, "can't allocate memory");
    return 0;
  }
  inf->owner = (*env)->NewWeakGlobalRef(env, obj);
  inf->isDaemon = (isDaemon==JNI_TRUE);

  const char *name = allocchars(env, clientName);
  fprintf(stderr, "opening jack client \"%s\"\n", name);
  jack_client_t *client = jack_client_open(name, JackNullOption, NULL);
  freechars(env, clientName, name);
  inf->client = client;
  inf->byteBufferArray[INPUT] = inf->byteBufferArray[OUTPUT] = NULL;
  if (client==0) {
    throwExc(env, "can't open client, jack server not running?");
    closeClient(env, obj, inf);
    return 0;
  }
  jack_set_process_callback(client, process, inf);
  jack_on_shutdown(client, shutdown, inf);

  int mode, i;
  char *portName = malloc(100);
  for (mode=INPUT; mode<=OUTPUT; mode++) {
    inf->portCount[mode] = (mode==INPUT) ? portsIn : portsOut;
    inf->byteBufferArray[mode] = (*env)->NewGlobalRef(env,
                (*env)->NewObjectArray(env,
                    (jsize)inf->portCount[mode], cls_ByteBuffer, NULL));
    for (i=0; i<inf->portCount[mode]; i++) {
      sprintf(portName, "%s_%i", MODE_LABEL[mode], (i+1));
      inf->port[mode][i] = jack_port_register(client, portName,
                  JACK_DEFAULT_AUDIO_TYPE, MODE_JACK[mode], 0);
      inf->sampleBuffers[mode][i] = NULL;
    }
  }
  free(portName);

  if (jack_activate(inf->client)) {
    throwExc(env, "cannot activate client");
    closeClient(env, obj, inf);
    return 0;
  }

  fprintf(stderr, "using %i input ports, %i output ports\n",
                      inf->portCount[INPUT], inf->portCount[OUTPUT]);

  return (jlong) inf;
}

int connectPorts(JNIEnv *env, jobject obj, jlong infPtr, jint port, jint range, jstring target, int mode) {
  if (target==NULL) return 0;
  fprintf(stderr, "connecting %s ports\n", MODE_LABEL[mode]);

  INF inf = (INF) infPtr;
  const char *targetPort = allocchars(env, target);
  int portFlags = (*targetPort) ? 0 : JackPortIsPhysical;
  const char **ports = jack_get_ports(inf->client, targetPort,
                              NULL, portFlags|MODE_JACK[1-mode]);
  freechars(env, target, targetPort);
  if (ports == NULL) return 0;

  int i;
  for (i=0; i<range; i++) {
    fprintf(stderr, "connecting %s %i\n", MODE_LABEL[mode], (port+i+1));
    if (ports[i]==NULL) {
      fprintf(stderr, "not enough ports to autoconnect to\n");
      break;
    }
    if (mode==INPUT) {
      if (jack_connect(inf->client, ports[i],
                    jack_port_name(inf->port[mode][port+i]))) {
        fprintf(stderr, "cannot autoconnect input port\n");
        break;
      }
    }
    else { // mode == OUTPUT
      if (jack_connect(inf->client,
                jack_port_name(inf->port[mode][port+i]), ports[i])) {
        fprintf(stderr, "cannot autoconnect output port\n");
        break;
      }
    }
  }
  free (ports);
  return i;
}

/*
 * private int connectInputPorts(long, int, int, String)
 *
 * target==NULL: no autoconnect
 * target=="":   autoconnect to physical ports
 * target==name: autoconnect to ports of named client
 */
JNIEXPORT jint JNICALL Java_com_noisepages_nettoyeur_jack_JackNativeClient_connectInputPorts(JNIEnv *env, jobject obj, jlong infPtr, jint port, jint range, jstring target) {
  return (jint) connectPorts(env, obj, infPtr, port, range, target, INPUT);
}

/*
 * private int connectOutputPorts(long, int, int, String)
 *
 * target==NULL: no autoconnect
 * target=="":   autoconnect to physical ports
 * target==name: autoconnect to ports of named client
 */
JNIEXPORT jint JNICALL Java_com_noisepages_nettoyeur_jack_JackNativeClient_connectOutputPorts(JNIEnv *env, jobject obj, jlong infPtr, jint port, jint range, jstring target) {
  return (jint) connectPorts(env, obj, infPtr, port, range, target, OUTPUT);
}

int disconnectPorts(JNIEnv *env, jobject obj, jlong infPtr, jint port, jint range, int mode) {
  fprintf(stderr, "port %d, range %d\n", port, range);
  INF inf = (INF) infPtr;
  int i;
  for(i=port; i<port+range; i++) {
    fprintf(stderr, "disconnecting %s port %d\n", MODE_LABEL[mode], i+1);
    if (jack_port_disconnect(inf->client, inf->port[mode][i])) {
      fprintf(stderr, "unable to disconnect port\n");
      break;
    }
  }
  return i-port;
}

/*
 * private int disconnectInputPorts(long, int, int)
 */
JNIEXPORT jint JNICALL Java_com_noisepages_nettoyeur_jack_JackNativeClient_disconnectInputPorts(JNIEnv *env, jobject obj, jlong infPtr, jint port, jint range) {
  return (jint) disconnectPorts(env, obj, infPtr, port, range, INPUT);
}

/*
 * private int disconnectOutputPorts(long, int, int)
 */
JNIEXPORT jint JNICALL Java_com_noisepages_nettoyeur_jack_JackNativeClient_disconnectOutputPorts(JNIEnv *env, jobject obj, jlong infPtr, jint port, jint range) {
  return (jint) disconnectPorts(env, obj, infPtr, port, range, OUTPUT);
}

/*
 * private void closeClient(long)
 */
JNIEXPORT void JNICALL Java_com_noisepages_nettoyeur_jack_JackNativeClient_closeClient(JNIEnv *env, jobject obj, jlong infPtr) {
  closeClient(env, obj, (INF) infPtr);
}

/*
 * private static native int getMaxPorts()
 */
JNIEXPORT jint JNICALL Java_com_noisepages_nettoyeur_jack_JackNativeClient_getMaxPorts(JNIEnv *env, jclass cls) {
  return (jint) MAX_PORTS;
}

/*
 * public static native int getSampleRate() throws JackException
 */
JNIEXPORT jint JNICALL Java_com_noisepages_nettoyeur_jack_JackNativeClient_getSampleRate(JNIEnv *env, jclass cls) {
  jack_client_t *client = jack_client_open("jack auxiliary client",
            JackNoStartServer, NULL);
  if (client!=NULL) {
    jint sampleRate = jack_get_sample_rate(client);
    jack_client_close(client);
    return sampleRate;
  } else {
    throwExc(env, "unable to open client; jack not running?");
    return 0;
  }
}

/*
 * public static native int getBufferSize() throws JackException
 */
JNIEXPORT jint JNICALL Java_com_noisepages_nettoyeur_jack_JackNativeClient_getBufferSize(JNIEnv *env, jclass cls) {
  jack_client_t *client = jack_client_open("jack auxiliary client",
            JackNoStartServer, NULL);
  if (client!=NULL) {
    jint bufSize = jack_get_buffer_size(client);
    jack_client_close(client);
    return bufSize;
  } else {
    throwExc(env, "unable to open client; jack not running?");
    return 0;
  }
}
