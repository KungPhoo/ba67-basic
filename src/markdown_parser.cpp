#include "markdown_parser.h"
#include "string_helper.h"
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <vector>

MarkdownParser::MarkdownParser(const std::string& filename)
    : filename(filename) {
}

void MarkdownParser::readMarkdown() {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    fileContent = ss.str();
}

void MarkdownParser::extractHeadingsAndUsage() {
    std::regex headingRegex(R"(^(#+)\s(.+))");
    std::regex usageRegex(R"(\*\*Usage:\*\*)");
    std::regex inlineCodeRegex(R"(`([^`]+)`)"), blockCodeRegex(R"(^```[a-zA-Z]*$([\s\S]*?)^```$)");

    std::istringstream stream(fileContent);
    std::string line;
    std::string lastHeading;
    int lastLevel        = 0;
    bool captureNextCode = false;
    bool insideTOC       = false;
    std::string pendingCode;
    bool insideBlockCode = false;

    while (std::getline(stream, line)) {
        if (line.find("<!-- TOC_START -->") != std::string::npos) {
            insideTOC = true;
            continue;
        }
        if (line.find("<!-- TOC_END -->") != std::string::npos) {
            insideTOC = false;
            continue;
        }
        if (insideTOC)
            continue; // Skip TOC lines

        std::smatch match;
        if (std::regex_match(line, match, headingRegex)) {
            lastLevel   = int(match[1].str().length());
            lastHeading = match[2].str();
            headings.emplace_back(lastHeading, lastLevel);
            captureNextCode = false;
        } else if (std::regex_search(line, match, usageRegex)) {
            captureNextCode = true;
            pendingCode.clear();
        }

        if (captureNextCode) {
            std::smatch codeMatch;
            if (std::regex_search(line, codeMatch, inlineCodeRegex)) {
                usageSections.emplace_back(lastHeading, codeMatch[1].str());
                captureNextCode = false;
            } else if (line.rfind("```", 0) == 0) { // Detect block code start
                insideBlockCode = true;
                pendingCode.clear();
            } else if (insideBlockCode) {
                if (line.rfind("```", 0) == 0) { // Detect block code end
                    usageSections.emplace_back(lastHeading, pendingCode);
                    captureNextCode = false;
                    insideBlockCode = false;
                } else {
                    pendingCode += line + "\n";
                }
            }
        }
    }
}
void MarkdownParser::updateTOC() {
    std::regex tocRegex(R"(<!-- TOC_START -->[\s\S]*?<!-- TOC_END -->)");
    std::ostringstream tocStream;
    tocStream << "<!-- TOC_START -->\n";
    for (const auto& heading : headings) {
        std::string anchor = heading.first;
        std::transform(anchor.begin(), anchor.end(), anchor.begin(), [](unsigned char c) {
            return std::isalnum(c) ? std::tolower(c) : '-';
        });
        tocStream << std::string(2 * (heading.second - 1), ' ') << "- [" << heading.first << "](#" << anchor << ")\n";
    }
    tocStream << "<!-- TOC_END -->";

    fileContent = std::regex_replace(fileContent, tocRegex, tocStream.str());

    std::ofstream outFile(filename);
    if (!outFile) {
        std::cerr << "Error writing to file: " << filename << std::endl;
        return;
    }
    outFile << fileContent;
}

void MarkdownParser::writeHelpInclude(const std::string& path) {
    std::ofstream help(path);
    if (!help) {
        std::cerr << "Error writing usage file." << std::endl;
        return;
    }
    help << "// DO NOT EDIT THIS FILE.\n";
    help << "// IT GETS GENERATED BY markdown_parser.cpp\n";
    help << "// WHEN RUNING A DEBUG BUILD.\n";

    bool first = true;

    for (const auto& entry : usageSections) {
        if (first) {
            first = false;
        } else {
            help << ",";
        }

        std::string usage = entry.second;
        // remove \r, escape in string
        StringHelper::replace(usage, "\r\n", "\n");
        StringHelper::replace(usage, "\r", "");
        StringHelper::replace(usage, "\n", "\\n");
        help << "{\"" << entry.first << "\", R\"RAW(" << usage << ")RAW\"}";
        help << "\n";
    }
}

void MarkdownParser::ParseAndApplyManual() {
    std::string sep(1, std::filesystem::path::preferred_separator);
    std::filesystem::path path = __FILE__;
    std::string dir            = path.parent_path().parent_path().string() + sep;
    std::string markdown       = dir + "readme.md";
    std::string helpfile       = dir + "src" + sep + "help.inc";

    MarkdownParser parser(markdown);
    parser.readMarkdown();
    parser.extractHeadingsAndUsage();
    parser.updateTOC();
    parser.writeHelpInclude(helpfile);
}
