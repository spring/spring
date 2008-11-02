#include "SelectMenu.h"

#include <SDL_keysym.h>
#include <SDL_timer.h>
#include <SDL_types.h>

#include "GameSetup.h"
#include "PreGame.h"
#include "Platform/Clipboard.h"
#include "Platform/ConfigHandler.h"
#include "LogOutput.h"
#include "Util.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/glFont.h"

using std::string;

extern Uint8* keys;
extern bool globalQuit;


SelectMenu::SelectMenu(bool server) : isServer(server)
{
	if (!isServer)
	{
		userInput=configHandler.GetString("address","");
		writingPos = userInput.length();
		userPrompt = "Enter server address: ";
		userWriting = true;
	}
}


int SelectMenu::KeyPressed(unsigned short k,bool isRepeat)
{
	if (k == SDLK_ESCAPE){
		if(keys[SDLK_LSHIFT]){
			logOutput.Print("User exited");
			globalQuit=true;
		} else
			logOutput.Print("Use shift-esc to quit");
	}

	if (userWriting) {
		keys[k] = true;
		if (k == SDLK_v && keys[SDLK_LCTRL]){
			CClipboard clipboard;
			const string text = clipboard.GetContents();
			userInput.insert(writingPos, text);
			writingPos += text.length();
			return 0;
		}
		if(k == SDLK_BACKSPACE){
			if (!userInput.empty() && (writingPos > 0)) {
				userInput.erase(writingPos - 1, 1);
				writingPos--;
			}
			return 0;
		}
		if (k == SDLK_DELETE) {
			if (!userInput.empty() && (writingPos < (int)userInput.size())) {
				userInput.erase(writingPos, 1);
			}
			return 0;
		}
		else if (k == SDLK_LEFT) {
			writingPos = std::max(0, std::min((int)userInput.length(), writingPos - 1));
		}
		else if (k == SDLK_RIGHT) {
			writingPos = std::max(0, std::min((int)userInput.length(), writingPos + 1));
		}
		else if (k == SDLK_HOME) {
			writingPos = 0;
		}
		else if (k == SDLK_END) {
			writingPos = (int)userInput.length();
		}
		if (k == SDLK_RETURN){
			userWriting = false;
			return 0;
		}
		return 0;
	}

	return 0;
}

bool SelectMenu::Draw()
{
	SDL_Delay(10); // milliseconds
	PrintLoadMsg("", false); // just clear screen and set up matrices etc.

	if (userWriting) {
		const std::string tempstring = userPrompt + userInput;

		const float xStart = 0.10f;
		const float yStart = 0.75f;

		const float fontScale = 1.0f;

		// draw the caret
		const int caretPos = userPrompt.length() + writingPos;
		const string caretStr = tempstring.substr(0, caretPos);
		const float caretWidth = font->CalcTextWidth(caretStr.c_str());
		char c = userInput[writingPos];
		if (c == 0) { c = ' '; }
		const float cw = fontScale * font->CalcCharWidth(c);
		const float csx = xStart + (fontScale * caretWidth);
		glDisable(GL_TEXTURE_2D);
		const float f = 0.5f * (1.0f + fastmath::sin((float)SDL_GetTicks() * 0.015f));
		glColor4f(f, f, f, 0.75f);
		glRectf(csx, yStart, csx + cw, yStart + fontScale * font->GetHeight());
		glEnable(GL_TEXTURE_2D);

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		font->glPrintAt(xStart, yStart, fontScale, tempstring.c_str());
	}

	return true;
}

bool SelectMenu::Update()
{
	if (isServer)
	{
		LocalSetup* mySettings = new LocalSetup();
		mySettings->myPlayerName = configHandler.GetString("name", "unnamed");
		mySettings->isHost = true;
		pregame = new CPreGame(mySettings, "", "");
		delete this;
	}
	else
	{
		// we are a client, wait for user to type address
		if (!userWriting)
		{
			configHandler.SetString("address",userInput);
			LocalSetup* mySettings = new LocalSetup();
			mySettings->myPlayerName = configHandler.GetString("name", "unnamed");
			mySettings->hostip = userInput;
			mySettings->isHost = false;
			pregame = new CPreGame(mySettings, "", "");
			delete this;
		}
	}


	return true;
}
