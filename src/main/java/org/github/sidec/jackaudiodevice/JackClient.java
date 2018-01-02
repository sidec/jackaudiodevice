package org.github.sidec.jackaudiodevice;

import com.noisepages.nettoyeur.jack.JackException;
import com.noisepages.nettoyeur.jack.JackNativeClient;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.FloatBuffer;
import java.nio.channels.Pipe;

public class JackClient extends JackNativeClient {
    private Pipe.SinkChannel sinkIn;
    private int portsOut;
    private Pipe.SourceChannel sourceOut;
    private int portsIn;
    public JackClient(JackAudioDevice jackAudioDevice) throws JackException {
        super(jackAudioDevice.getName(), jackAudioDevice.getPortsIn(), jackAudioDevice.getPortsOut());
        sinkIn = jackAudioDevice.getSinkIn();
        sourceOut = jackAudioDevice.getSourceOut();
        portsIn = jackAudioDevice.getPortsIn();
        portsOut = jackAudioDevice.getPortsOut();
    }

    @Override
    protected void process(FloatBuffer[] inBuffers, FloatBuffer[] outBuffers) {

        ByteBuffer buf = ByteBuffer.allocate(outBuffers.length * Double.BYTES * outBuffers[0].capacity());
        try {
            int bytesRead =
                    sourceOut.read(buf);
            buf.rewind();
            while (buf.hasRemaining()){
                for(int i = 0; i < portsOut; i++){
                    float value = (float)buf.getDouble();
                    outBuffers[i].put(value);
                }
            }
            for(int i = 0; i < portsOut; i++){
                outBuffers[i].flip();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        if(portsIn>0){
        ByteBuffer bufIn = ByteBuffer.allocate(inBuffers.length * Float.BYTES * inBuffers[0].capacity());
        bufIn.clear();
        while (bufIn.hasRemaining()){
            for (FloatBuffer inBuffer : inBuffers) {
                float value = inBuffer.get();
                bufIn.putFloat(value);
            }
        }
        bufIn.flip();
        try {
            int wroteBytes = sinkIn.write(bufIn);
        } catch (IOException e) {
            e.printStackTrace();
        }
        }


    }

    void setSinkIn(Pipe.SinkChannel sinkIn) {
        this.sinkIn = sinkIn;
    }

    void setSourceOut(Pipe.SourceChannel sourceOut) {
        this.sourceOut = sourceOut;
    }
}
