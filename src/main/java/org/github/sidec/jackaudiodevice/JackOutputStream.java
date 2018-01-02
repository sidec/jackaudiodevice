package org.github.sidec.jackaudiodevice;

import com.jsyn.devices.AudioDeviceOutputStream;
import com.noisepages.nettoyeur.jack.JackException;
import com.noisepages.nettoyeur.jack.JackNativeClient;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.Pipe;

public class JackOutputStream extends JackStream implements AudioDeviceOutputStream {

    private final int portsOut;
    private final Pipe.SinkChannel sinkOut;
    private final int bufferSize;

    public JackOutputStream(JackAudioDevice jackAudioDevice) throws JackException {
        super(jackAudioDevice);

        portsOut = jackAudioDevice.getPortsOut();
        sinkOut = jackAudioDevice.getSinkOut();
        bufferSize = JackNativeClient.getBufferSize();
    }


        @Override
        public void write(double value) throws IOException {
            write(new double[]{value});
        }

        @Override
        public void write(double[] buffer) throws IOException {
            ByteBuffer bufferOut = ByteBuffer.allocate(portsOut * bufferSize * 8);
            bufferOut.clear();
            for (double v : buffer) {
                bufferOut.putDouble(v);
            }
            bufferOut.flip();
            sinkOut.write(bufferOut);
        }

       @Override
        public void write(double[] buffer, int start, int count) throws IOException {
            final double[] sub = new double[count];
            System.arraycopy(buffer, start, sub, 0, count);
            write(sub);
        }

}
