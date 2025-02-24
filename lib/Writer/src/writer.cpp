#include "writer.h"

Sd2Card card;
SdVolume volume;
SdFile root;

void initWriter(uint8_t p_pin) {
    card.init(SPI_HALF_SPEED, p_pin);
    volume.init(card);
    root.openRoot(volume);
}

SdFile openFile() {
    char filename[9] = {'0', '0', '0', '0', '.', 't', 'x', 't', 0};

    for (uint16_t i = 0; i < 10000; i++) {
        uint16_t curr = i;
        uint8_t index = 3;
        do {
            filename[index] = '0' + (curr % 10);
            curr /= 10;
            index -= 1;
        } while (curr > 0 && index < 3);

        // Check for existence
        SdFile child;
        if (child.open(root, filename, O_RDONLY)) {
            child.close();
        } else {
            break;
        }
    }

    SdFile file;
    if (!file.open(root, filename, O_WRITE | O_CREAT)) {
        return SdFile();
    }
    return file;
}

SdFile openFile(DateTime ts) {
    char dirname[9];   // 8 + '\0'
    char filename[13]; // 8 + '.' + 3 + '\0'
    sprintf(dirname, "%04d%02d%02d", ts.year(), ts.month(), ts.day());

    SdFile dir;
    if (dir.open(root, dirname, O_RDONLY)) {
        // All good
    } else {
        if (!dir.makeDir(root, dirname))
            goto openFile_error;
        if (!dir.open(root, dirname, O_RDONLY))
            goto openFile_error;
    }

    {
        sprintf(&filename[8], "/%02d-%02d-%02d.txt", ts.hour(), ts.minute(), ts.second());

        SdFile file;
        const uint8_t success = file.open(dir, filename, O_WRITE | O_CREAT);
        dir.close();

        if (!success)
            goto openFile_error;

        return file;
    }
openFile_error:
    return SdFile();
}

void writeToFile(SdFile& p_file, const char* p_text) {
    p_file.clearWriteError();
    p_file.write(p_text, strlen(p_text));
}

void writeToFile(SdFile& p_file, uint8_t p_number) {
    char buffer[4];
    int ret = sprintf(&buffer[0], "%d", p_number);
    if (ret) {
        writeToFile(p_file, buffer);
    }
}

void closeFile(SdFile& p_file) {
    p_file.close();
}
