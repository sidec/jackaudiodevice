package org.github.sidec.jackaudiodevice;

import com.jsyn.devices.AudioDeviceInputStream;
import com.noisepages.nettoyeur.jack.JackException;
import com.noisepages.nettoyeur.jack.JackNativeClient;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.Pipe;

public class JackInputStream  extends JackStream implements AudioDeviceInputStream {

    private final Pipe.SourceChannel sourceIn;
    private final int bufSize;

    public JackInputStream(JackAudioDevice jackAudioDevice) throws JackException {
        super(jackAudioDevice);

        sourceIn = jackAudioDevice.getSourceIn();
        bufSize = JackNativeClient.getBufferSize();
    }

    @Override
    public double read() {
        System.out.println("read()");
        return bufSize;
    }

    /**
     * Try to fill the entire buffer.
     *
     * @param buffer
     * @return number of samples read
     */
    @Override
    public int read(double[] buffer) {
        int readFrom = 0;
        ByteBuffer inBuffer = ByteBuffer.allocate(buffer.length*Float.BYTES);
        inBuffer.clear();
        try {
            readFrom = sourceIn.read(inBuffer);
            inBuffer.flip();
            for(int i = 0; i < buffer.length; i++){
               buffer[i] = inBuffer.getFloat();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return readFrom;
    }

    /**
     * Read from the stream. Block until some data is available.
     *
     * @param buffer
     * @param start  index of first sample in buffer
     * @param count  number of samples to read, for example count=8 for 4 stereo frames
     * @return number of samples read
     */
    @Override
    public int read(double[] buffer, int start, int count) {
        System.out.println("read");
        return bufSize;
    }

    /**
     * @return number of samples currently available to read without blocking
     */
    @Override
    public int available() {
        return bufSize;
    }
}
