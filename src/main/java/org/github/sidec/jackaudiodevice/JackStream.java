package org.github.sidec.jackaudiodevice;

public class JackStream {
    final private JackAudioDevice jackAudioDevice;
    public JackStream(JackAudioDevice jackAudioDevice) {
        this.jackAudioDevice = jackAudioDevice;
    }

    public void close(){

    }
    public void start(){
        jackAudioDevice.getClient().connectOutputPorts("system");
        jackAudioDevice.getClient().connectInputPorts("system");
    }

    public void stop(){

    }

    public double getLatency() {
        return 0;
    }
}
