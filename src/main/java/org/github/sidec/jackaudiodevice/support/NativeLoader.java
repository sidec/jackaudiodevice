package org.github.sidec.jackaudiodevice.support;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.logging.Logger;

import static java.nio.file.StandardCopyOption.REPLACE_EXISTING;

public interface NativeLoader {

    Logger log = Logger.getAnonymousLogger();

    static void load(Class c) {

        String libraryName = System.mapLibraryName(c.getSimpleName());

        String path = File.separator + "native" +
                File.separator + System.getProperty("os.name") +
                File.separator + System.getProperty("os.arch") +
                File.separator + libraryName;

        try {
            Path tempFile = Files.createTempFile(NativeLoader.class.getSimpleName()+"-","-"+libraryName);
            log.info(tempFile.toString());
            Files.copy(NativeLoader.class.getResourceAsStream(path), tempFile, REPLACE_EXISTING);
            tempFile.toFile().deleteOnExit();
            System.load(tempFile.toString());
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

}


