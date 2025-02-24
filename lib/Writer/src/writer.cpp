#include "writer.h"

void initWriter(uint8_t p_pin) {
    SD.begin(p_pin);
}

File openFile() {
    char filename[9] = {'0', '0', '0', '0', '.', 't', 'x', 't', 0};

    for (uint16_t i = 0; i < 10000; i++) {
        uint16_t curr = i;
        uint8_t index = 3;
        do {
            filename[index] = '0' + (curr % 10);
            curr /= 10;
            index -= 1;
        } while (curr > 0 && index < 3);

        if (!SD.exists(filename)) {
            break;
        }
    }

    return SD.open(filename, FILE_WRITE);
}

File openFile(DateTime ts) {
    char* filename = "YYYYMMDD\0HH-MM-SS.txt";
    sprintf(filename, "%04d%02d%02d", ts.year(), ts.month(), ts.day());
    if (!SD.exists(filename)) {
        SD.mkdir(filename);
    }
    sprintf(&filename[8], "/%02d-%02d-%02d", ts.hour(), ts.minute(), ts.month());
    return SD.open(filename, FILE_WRITE);
}

void writeToFile(File* p_file, const char* p_text) {
    if (nullptr == p_file) {
        return;
    }

    p_file->print(p_text);
}

void writeToFile(File* p_file, uint8_t p_number) {
    if (nullptr == p_file) {
        return;
    }

    char buffer[4];
    int ret = sprintf(&buffer[0], "%d", p_number);
    if (ret) {
        p_file->print(buffer);
    }
}

void closeFile(File* p_file) {
    if (nullptr == p_file) {
        return;
    }

    p_file->close();
}
