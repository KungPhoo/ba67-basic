// Very, Very, very primitive ABC music notation parser
// and converter to WAV file.
#include <map>
#include <string>
#include <vector>

class AbcMusic {
private:
    // Constants for WAV file format
    const int SAMPLE_RATE       = 44100;
    const short NUM_CHANNELS    = 1;
    const short BITS_PER_SAMPLE = 16;
    const int MAX_AMPLITUDE     = 32760; // Maximum amplitude for 16-bit PCM

    // Frequency map for notes (C4, D4, E4, etc.)
    std::map<std::string, double> noteFreqs = {
        {  "C4", 261.63 },
        { "C#4", 277.18 },
        {  "D4", 293.66 },
        { "D#4", 311.13 },
        {  "E4", 329.63 },
        {  "F4", 349.23 },
        { "F#4", 369.99 },
        {  "G4", 392.00 },
        { "G#4", 415.30 },
        {  "A4", 440.00 },
        { "A#4", 466.16 },
        {  "B4", 493.88 },
        {  "C5", 523.25 },
        { "C#5", 554.37 },
        {  "D5", 587.33 },
        { "D#5", 622.25 },
        {  "E5", 659.26 },
        {  "F5", 698.46 },
        { "F#5", 739.99 },
        {  "G5", 783.99 },
        { "G#5", 830.61 },
        {  "A5", 880.00 },
        { "A#5", 932.33 },
        {  "B5", 987.77 },

        {   "c", 261.63 },
        {  "c#", 277.18 },
        {   "d", 293.66 },
        {  "d#", 311.13 },
        {   "e", 329.63 },
        {   "f", 349.23 },
        {  "f#", 369.99 },
        {   "g", 392.00 },
        {  "g#", 415.30 },
        {   "a", 440.00 },
        {  "a#", 466.16 },
        {   "b", 493.88 },
        {   "C", 523.25 },
        {  "C#", 554.37 },
        {   "D", 587.33 },
        {  "D#", 622.25 },
        {   "E", 659.26 },
        {   "F", 698.46 },
        {  "F#", 739.99 },
        {   "G", 783.99 },
        {  "G#", 830.61 },
        {   "A", 880.00 },
        {  "A#", 932.33 },
        {   "B", 987.77 },
    };

    // Function to generate a sine wave for a given frequency and duration
    std::vector<short> generateSineWave(double frequency, double duration);

    // Function to parse ABC notation and return a list of notes and their durations
    std::vector<std::pair<std::string, double>> parseABCNotation(const std::string& abc);

    // Function to write a WAV file with given samples
    void writeWAVFile(const std::string& filename, const std::vector<short>& samples);

public:
    bool generate(const std::string& abc, const std::string& wavPathUtf8);
};