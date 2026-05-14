constexpr char *GEMINI_URL =
    "https://generativelanguage.googleapis.com/v1beta/models/"
    "gemini-3-flash-preview:generateContent";

constexpr char *INSTRUCTIONS =
    "You are a helpful assistant that generates Anki flashcards based on the "
    "provided image. "
    "Extract the English word/concept and its Russian translation from "
    "this Duolingo exercise screenshot. Provide a short, concise card "
    "name. If one word in the screenshot is unknown, it will be inside an "
    "extra speech bubble.";
