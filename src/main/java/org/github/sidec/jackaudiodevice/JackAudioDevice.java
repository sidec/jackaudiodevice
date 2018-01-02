package org.github.sidec.jackaudiodevice;

import com.jsyn.JSyn;
import com.jsyn.Synthesizer;
import com.jsyn.devices.AudioDeviceInputStream;
import com.jsyn.devices.AudioDeviceManager;
import com.jsyn.devices.AudioDeviceOutputStream;
import com.jsyn.unitgen.ChannelIn;
import com.jsyn.unitgen.LineOut;
import com.jsyn.unitgen.SineOscillator;
import com.noisepages.nettoyeur.jack.JackException;

import java.io.IOException;
import java.nio.channels.Pipe;

public class JackAudioDevice  implements AudioDeviceManager {
    private final int portsIn;
    private final int portsOut;
    private final Pipe.SinkChannel sinkOut;
    private final Pipe.SourceChannel sourceOut;
    private final JackOutputStream jackOutputStream;
    private final JackInputStream jackInputStream;
    private final Pipe.SinkChannel sinkIn;
    private final Pipe.SourceChannel sourceIn;
    private final String jackClientName;
    private JackClient client;

    public JackAudioDevice(final String jackClientName, int portsIn, int portsOut) throws JackException, IOException {
        this.jackClientName = jackClientName;
        this.portsIn = portsIn;
        this.portsOut = portsOut;

        Pipe pipeOut = Pipe.open();
        sinkOut = pipeOut.sink();
        sourceOut = pipeOut.source();

        Pipe pipeIn = Pipe.open();
        sinkIn = pipeIn.sink();
        sourceIn = pipeIn.source();

        this.jackOutputStream = new JackOutputStream(this);
        this.jackInputStream = new JackInputStream(this);

        client = new JackClient(this);
        client.setSinkIn(sinkIn);
        client.setSourceOut(sourceOut);
        int bufferSize = client.getBufferSize();
    }



    public JackClient getClient() {
        return client;
    }

    public void setClient(JackClient client) {
        this.client = client;
    }

    /**
     * @return The number of devices available.
     */
    @Override
    public int getDeviceCount() {
        return 1;
    }

    /**
     * Get the name of an audio device.
     *
     * @param deviceID An index between 0 to deviceCount-1.
     * @return A name that can be shown to the user.
     */
    @Override
    public String getDeviceName(int deviceID) {
        return "system";
    }

    /**
     * @return A name of the device manager that can be shown to the user.
     */
    @Override
    public String getName() {
        return jackClientName;
    }

    /**
     * The user can generally select a default device using a control panel that is part of the
     * operating system.
     *
     * @return The ID for the input device that the user has selected as the default.
     */
    @Override
    public int getDefaultInputDeviceID() {
        return 0;
    }

    /**
     * The user can generally select a default device using a control panel that is part of the
     * operating system.
     *
     * @return The ID for the left device that the user has selected as the default.
     */
    @Override
    public int getDefaultOutputDeviceID() {
        return 0;
    }

    /**
     * @param deviceID
     * @return The maximum number of channels that the device will support.
     */
    @Override
    public int getMaxInputChannels(int deviceID) {
        return 64;
    }

    /**
     * @param deviceID An index between 0 to numDevices-1.
     * @return The maximum number of channels that the device will support.
     */
    @Override
    public int getMaxOutputChannels(int deviceID) {
        return 64;
    }

    /**
     * This the lowest latency that the device can support reliably. It should be used for
     * applications that require low latency such as live processing of guitar signals.
     *
     * @param deviceID An index between 0 to numDevices-1.
     * @return Latency in seconds.
     */
    @Override
    public double getDefaultLowInputLatency(int deviceID) {
        return 0;
    }

    /**
     * This the highest latency that the device can support. High latency is recommended for
     * applications that are not time critical, such as recording.
     *
     * @param deviceID An index between 0 to numDevices-1.
     * @return Latency in seconds.
     */
    @Override
    public double getDefaultHighInputLatency(int deviceID) {
        return 0;
    }

    @Override
    public double getDefaultLowOutputLatency(int deviceID) {
        return 0;
    }

    @Override
    public double getDefaultHighOutputLatency(int deviceID) {
        return 0;
    }

    /**
     * Set latency in seconds for the audio device. If set to zero then the DefaultLowLatency value
     * for the device will be used. This is just a suggestion that will be used when the
     * AudioDeviceInputStream is started.
     *
     * @param latency
     */
    @Override
    public int setSuggestedInputLatency(double latency) {
        return 0;
    }

    @Override
    public int setSuggestedOutputLatency(double latency) {
        return 0;
    }

    /**
     * Create a stream that can be used internally by JSyn for outputting audio data. Applications
     * should not call this directly.
     *
     * @param deviceID
     * @param frameRate
     * @param numOutputChannels
     */
    @Override
    public AudioDeviceOutputStream createOutputStream(int deviceID, int frameRate, int numOutputChannels) {
        return jackOutputStream;
    }

    /**
     * Create a stream that can be used internally by JSyn for acquiring audio input data.
     * Applications should not call this directly.
     *
     * @param deviceID
     * @param frameRate
     * @param numInputChannels
     */
    @Override
    public AudioDeviceInputStream createInputStream(int deviceID, int frameRate, int numInputChannels) {
        return jackInputStream;
    }

    int getPortsIn() {
        return portsIn;
    }

    int getPortsOut() {
        return portsOut;
    }

    Pipe.SinkChannel getSinkOut() {
        return sinkOut;
    }

    Pipe.SourceChannel getSourceOut() {
        return sourceOut;
    }

    Pipe.SinkChannel getSinkIn() {
        return sinkIn;
    }

    Pipe.SourceChannel getSourceIn() {
        return sourceIn;
    }
}
