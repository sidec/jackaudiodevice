package org.github.sidec.jackaudiodevice;


import com.jsyn.JSyn;
import com.jsyn.Synthesizer;
import com.jsyn.unitgen.ChannelIn;
import com.jsyn.unitgen.LineOut;
import com.jsyn.unitgen.SineOscillator;
import com.noisepages.nettoyeur.jack.JackException;

import java.io.IOException;

public class JackAudioDeviceExample {


    public static void main(String... args) throws JackException, IOException {

        Synthesizer syn = JSyn.createSynthesizer(
                new JackAudioDevice("jack manager", 2, 2));

        SineOscillator sine = new SineOscillator(220) {{
            syn.add(this);
        }};

        ChannelIn channelIn = new ChannelIn(0){{
            syn.add(this);
        }};

        new LineOut() {{
            syn.add(this);
            input.connect(0, channelIn.output, 0);
            input.connect(1, sine.output, 0);
            start();
        }};

        syn.start(syn.getFrameRate() , 0, 2, 0, 2);
    }
}