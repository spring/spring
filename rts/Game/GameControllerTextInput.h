/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GAME_CONTROLLER_TEXTINPUT_H
#define GAME_CONTROLLER_TEXTINPUT_H

#include <string>
#include <vector>

#include <SDL2/SDL_rect.h>

class Action;
struct GameControllerTextInput {
public:
	void ViewResize();
	void Draw();

	void SetPos(float px, float py) {
		inputTextPosX  = px;
		inputTextPosY  = py;
	}
	void SetSize(float sx, float sy) {
		inputTextSizeX = sx;
		inputTextSizeY = sy;
	}


	int SetInputText(const std::string& utf8Text);
	int SetEditText(const std::string& utf8Text) {
		if (!userWriting)
			return 0;

		editText = utf8Text;
		editingPos = utf8Text.length();
		return 0;
	}


	void PromptInput(const std::string* inputPrefix) {
		if (inputPrefix != nullptr)
			userInputPrefix = *inputPrefix;

		userWriting = true;
		userChatting = true;
		ignoreNextChar = false;

		userPrompt = "Say: ";
		userInput = userInputPrefix;
		writingPos = (int)userInput.length();
	}

	void PromptLabel() {
		userWriting = true;
		userPrompt = "Label: ";
		ignoreNextChar = true;
	}


	bool SendPromptInput();
	bool SendLabelInput();
	void ClearInput() {
		userChatting = false;

		userInput = "";
		writingPos = 0;
	}


	bool CheckHandlePasteCommand(const std::string& rawLine);

	bool ConsumePressedKey(int key, const std::vector<Action>& actions);
	bool ConsumeReleasedKey(int key) const;

private:
	void PasteClipboard();

	bool HandleChatCommand(int key, const std::string& command);
	bool HandleEditCommand(int key, const std::string& command);
	bool HandlePasteCommand(int key, const std::string& rawLine);
	bool ProcessKeyPressAction(int key, const Action& action);

public:
	/// current writing/editing positions
	int writingPos = 0;
	int editingPos = 0;

	float inputTextPosX = 0.0f;
	float inputTextPosY = 0.0f;
	float inputTextSizeX = 0.0f;
	float inputTextSizeY = 0.0f;

	bool userWriting = false; // true for any text-input (chat, labels)
	bool userChatting = false; // true for chat-actions

	bool ignoreNextChar = false;
	bool drawTextCaret = true;

	SDL_Rect textEditingWindow;

	std::string userInput;
	std::string userInputPrefix;
	std::string userPrompt;
	std::string editText;
};

extern GameControllerTextInput gameTextInput;

#endif

