#!/usr/bin/env bash
gcc -fPIC --shared \
    -I${JAVA_HOME}/include/ -I${JAVA_HOME}/include/linux \
    -I/usr/include/jack \
    -o src/main/resources/native/Linux/amd64/libJackNativeClient.so src/main/c/com_noisepages_nettoyeur_jack_JackNativeClient.c \
    `pkg-config --cflags --libs jack`
