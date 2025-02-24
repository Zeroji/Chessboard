#pragma once

#include <RTClib.h>
#include <SD.h>
#include <stdint.h>

void initWriter(uint8_t p_pin);

// Open a file following the format "0000.txt"
// If the file already exists in the SD, try the next one: "0001.txt", then "0002.txt", etc
File openFile();

// Open a file following the format "YYYYMMDD/HH-MM-SS.txt"
File openFile(DateTime ts);

void writeToFile(File* p_file, const char* p_text);
void writeToFile(File* p_file, uint8_t p_number);
void closeFile(File* p_file);
