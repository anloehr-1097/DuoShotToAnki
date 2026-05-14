#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_

constexpr const char* GEMINI_URL =
    "https://generativelanguage.googleapis.com/v1beta/models/"
    "gemini-3-flash-preview:generateContent";

constexpr const char* INSTRUCTIONS =
    "You are a helpful assistant that generates Anki flashcards based on the "
    "provided image. "
    "Extract the English word/concept and its Russian translation from "
    "this Duolingo exercise screenshot. Provide a short, concise card "
    "name. If one word in the screenshot is unknown, it will be inside an "
    "extra speech bubble, pointing at the word. The bubble contains the word and the translation."
    "In this case, only translate the word, not the entire sentence.";

#endif  // SRC_CONFIG_H_
