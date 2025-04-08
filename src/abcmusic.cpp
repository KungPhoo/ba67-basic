#include "abcmusic.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// Function to generate a sine wave for a given frequency and duration
std::vector<short> AbcMusic::generateSineWave(double frequency, double duration) {
    int numSamples = static_cast<int>(SAMPLE_RATE * duration);
    std::vector<short> samples(numSamples);

    for (int i = 0; i < numSamples; ++i) {
        double time      = static_cast<double>(i) / SAMPLE_RATE;
        double amplitude = MAX_AMPLITUDE * std::sin(2 * 3.14158 * frequency * time);
        samples[i]       = static_cast<short>(amplitude);
    }

    return samples;
}

// Function to parse ABC notation and return a list of notes and their durations
std::vector<std::pair<std::string, double>> AbcMusic::parseABCNotation(const std::string& abc) {
    std::vector<std::pair<std::string, double>> notes;
    std::istringstream stream(abc);
    std::string token;

    while (stream >> token) {
        std::string note;
        double duration = 0.25; // Default duration [sec]

        // Check for duration (after the note)
        size_t pos = token.find_first_of("0123456789");
        if (pos != std::string::npos) {
            note     = token.substr(0, pos);
            duration = std::stod(token.substr(pos));
        } else {
            note = token; // No duration specified
        }

        // Adjust octave based on commas and apostrophes
        int octaveShift = 0;
        for (char c : note) {
            if (c == '\'') {
                octaveShift++; // Raise octave
            } else if (c == ',') {
                octaveShift--; // Lower octave
            } else {
                break;
            }
        }

        // Remove octave modifiers (commas and apostrophes)
        note = note.substr(octaveShift);

        // Find the corresponding frequency
        if (noteFreqs.find(note) != noteFreqs.end()) {
            double frequency = noteFreqs[note] * std::pow(2, octaveShift); // Apply octave shift
            notes.push_back({ note, duration });
        }
    }

    return notes;
}

// Function to write a WAV file with given samples
void AbcMusic::writeWAVFile(const std::string& filename, const std::vector<short>& samples) {
    std::ofstream file(filename, std::ios::binary);

    // Write WAV header (simplified version for mono 16-bit PCM)
    file.write("RIFF", 4); // Chunk ID
    int chunkSize = int(36 + samples.size() * sizeof(short)); // Chunk size
    file.write(reinterpret_cast<char*>(&chunkSize), 4); // Chunk size
    file.write("WAVE", 4); // Format
    file.write("fmt ", 4); // Subchunk 1 ID
    int subchunk1Size = 16; // PCM format size
    file.write(reinterpret_cast<char*>(&subchunk1Size), 4); // Subchunk 1 size
    short audioFormat = 1; // PCM format
    file.write(reinterpret_cast<const char*>(&audioFormat), 2);
    file.write(reinterpret_cast<const char*>(&NUM_CHANNELS), 2); // Mono
    file.write(reinterpret_cast<const char*>(&SAMPLE_RATE), 4);
    int byteRate = SAMPLE_RATE * NUM_CHANNELS * (BITS_PER_SAMPLE / 8);
    file.write(reinterpret_cast<char*>(&byteRate), 4);
    short blockAlign = NUM_CHANNELS * (BITS_PER_SAMPLE / 8);
    file.write(reinterpret_cast<const char*>(&blockAlign), 2);
    file.write(reinterpret_cast<const char*>(&BITS_PER_SAMPLE), 2); // 16 bits per sample

    file.write("data", 4); // Subchunk 2 ID
    int subchunk2Size = int(samples.size() * sizeof(short)); // Data size
    file.write(reinterpret_cast<const char*>(&subchunk2Size), 4); // Data size
    file.write(reinterpret_cast<const char*>(samples.data()), subchunk2Size); // Audio samples
}

bool AbcMusic::generate(const std::string& abc, const std::string& wavPathUtf8) {
    // Parse ABC notation
    auto notes = parseABCNotation(abc);

    // Generate sine wave for each note and combine them
    std::vector<short> samples;
    for (const auto& note : notes) {
        const auto& noteName = note.first;
        double duration      = note.second;

        if (noteFreqs.find(noteName) != noteFreqs.end()) {
            double frequency = noteFreqs[noteName];
            auto sineWave    = generateSineWave(frequency, duration);
            samples.insert(samples.end(), sineWave.begin(), sineWave.end());
        }
    }
    // Write WAV file
    writeWAVFile(wavPathUtf8, samples);
    return true;
}
