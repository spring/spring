/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "HUDDrawer.h"

#include "Rendering/Fonts/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDef.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/myMath.h"


HUDDrawer* HUDDrawer::GetInstance()
{
	static HUDDrawer hud;
	return &hud;
}

void HUDDrawer::PushState()
{
	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT);
	glPushMatrix();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
}
void HUDDrawer::PopState()
{
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	glPopMatrix();
	glPopAttrib();
}

void HUDDrawer::DrawModel(const CUnit* unit)
{
	glPushMatrix();
		glMatrixMode(GL_PROJECTION);
			glTranslatef(-0.8f, -0.4f, 0.0f);
			glMultMatrixf(camera->GetProjectionMatrix());
		glMatrixMode(GL_MODELVIEW);

		glTranslatef(0.0f, 0.0f, -unit->radius);
		glScalef(1.0f / unit->radius, 1.0f / unit->radius, 1.0f / unit->radius);

		if (unit->moveType->useHeading) {
			glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
			glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
		} else {
			CMatrix44f m(ZeroVector,
				float3(camera->GetRight().x, camera->GetUp().x, camera->GetDir().x),
				float3(camera->GetRight().y, camera->GetUp().y, camera->GetDir().y),
				float3(camera->GetRight().z, camera->GetUp().z, camera->GetDir().z));
			glMultMatrixf(m.m);
		}

		glColor4f(1.0f, 1.0f, 1.0f, 0.25f);
		unit->localModel.Draw();
	glPopMatrix();
}

void HUDDrawer::DrawUnitDirectionArrow(const CUnit* unit)
{
	glDisable(GL_TEXTURE_2D);

	if (unit->moveType->useHeading) {
		glPushMatrix();
			glTranslatef(-0.8f, -0.4f, 0.0f);
			glScalef(0.33f, 0.33f * globalRendering->aspectRatio, 0.33f);
			glRotatef(unit->heading * 180.0f / 32768 + 180, 0.0f, 0.0f, 1.0f);

			glColor4f(0.3f, 0.9f, 0.3f, 0.4f);
			glBegin(GL_TRIANGLE_FAN);
				glVertex2f(-0.2f, -0.3f);
				glVertex2f(-0.2f,  0.3f);
				glVertex2f( 0.0f,  0.4f);
				glVertex2f( 0.2f,  0.3f);
				glVertex2f( 0.2f, -0.3f);
				glVertex2f(-0.2f, -0.3f);
			glEnd();
		glPopMatrix();
	}
}
void HUDDrawer::DrawCameraDirectionArrow(const CUnit* unit)
{
	glDisable(GL_TEXTURE_2D);

	if (unit->moveType->useHeading) {
		glPushMatrix();
			glTranslatef(-0.8f, -0.4f, 0.0f);
			glScalef(0.33f, 0.33f * globalRendering->aspectRatio, 0.33f);

			glRotatef(
				GetHeadingFromVector(camera->GetDir().x, camera->GetDir().z) * 180.0f / 32768 + 180,
				0.0f, 0.0f, 1.0f
			);
			glScalef(0.4f, 0.4f, 0.3f);

			glColor4f(0.4f, 0.4f, 1.0f, 0.6f);
			glBegin(GL_TRIANGLE_FAN);
				glVertex2f(-0.2f, -0.3f);
				glVertex2f(-0.2f,  0.3f);
				glVertex2f( 0.0f,  0.5f);
				glVertex2f( 0.2f,  0.3f);
				glVertex2f( 0.2f, -0.3f);
				glVertex2f(-0.2f, -0.3f);
			glEnd();
		glPopMatrix();
	}
}

void HUDDrawer::DrawWeaponStates(const CUnit* unit)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_TEXTURE_2D);
	glColor4f(0.2f, 0.8f, 0.2f, 0.8f);
	font->glFormat(-0.9f, 0.35f, 1.0f, FONT_SCALE | FONT_NORM, "Health: %.0f / %.0f", (float) unit->health, (float) unit->maxHealth);

	if (playerHandler->Player(gu->myPlayerNum)->fpsController.mouse2) {
		font->glPrint(-0.9f, 0.30f, 1.0f, FONT_SCALE | FONT_NORM, "Free-Fire Mode");
	}

	int numWeaponsToPrint = 0;

	for (unsigned int a = 0; a < unit->weapons.size(); ++a) {
		const WeaponDef* wd = unit->weapons[a]->weaponDef;
		if (!wd->isShield) {
			++numWeaponsToPrint;
		}
	}

	if (numWeaponsToPrint > 0) {
		// we have limited space to draw whole list of weapons
		const float yMax = 0.25f;
		const float yMin = 0.00f;
		const float maxLineHeight = 0.045f;
		const float lineHeight = std::min((yMax - yMin) / numWeaponsToPrint, maxLineHeight);
		const float fontSize = 1.2f * (lineHeight / maxLineHeight);
		float yPos = yMax;

		for (unsigned int a = 0; a < unit->weapons.size(); ++a) {
			const CWeapon* w = unit->weapons[a];
			const WeaponDef* wd = w->weaponDef;

			if (!wd->isShield) {
				yPos -= lineHeight;

				if (wd->stockpile && !w->numStockpiled) {
					if (w->numStockpileQued) {
						glColor4f(0.8f, 0.2f, 0.2f, 0.8f);
						font->glFormat(-0.9f, yPos, fontSize, FONT_SCALE | FONT_NORM, "%s: Stockpiling (%i%%)", wd->description.c_str(), int(100.0f * w->buildPercent + 0.5f));
					}
					else {
						glColor4f(0.8f, 0.2f, 0.2f, 0.8f);
						font->glFormat(-0.9f, yPos, fontSize, FONT_SCALE | FONT_NORM, "%s: No ammo", wd->description.c_str());
					}
				} else if (w->reloadStatus > gs->frameNum) {
					glColor4f(0.8f, 0.2f, 0.2f, 0.8f);
					font->glFormat(-0.9f, yPos, fontSize, FONT_SCALE | FONT_NORM, "%s: Reloading (%i%%)", wd->description.c_str(), 100 - int(100.0f * (w->reloadStatus - gs->frameNum) / int(w->reloadTime / unit->reloadSpeed) + 0.5f));
				} else if (!w->angleGood) {
					glColor4f(0.6f, 0.6f, 0.2f, 0.8f);
					font->glFormat(-0.9f, yPos, fontSize, FONT_SCALE | FONT_NORM, "%s: Aiming", wd->description.c_str());
				} else {
					glColor4f(0.2f, 0.8f, 0.2f, 0.8f);
					font->glFormat(-0.9f, yPos, fontSize, FONT_SCALE | FONT_NORM, "%s: Ready", wd->description.c_str());
				}
			}
		}
	}
}

void HUDDrawer::DrawTargetReticle(const CUnit* unit)
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	// draw the reticle in world coordinates
	glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMultMatrixf(camera->GetProjectionMatrix());
	glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixf(camera->GetViewMatrix());

	glPushMatrix();

		for (unsigned int a = 0; a < unit->weapons.size(); ++a) {
			const CWeapon* w = unit->weapons[a];

			if (!w) {
				continue;
			}
			switch (a) {
				case 0:
					glColor4f(0.0f, 1.0f, 0.0f, 0.7f);
					break;
				case 1:
					glColor4f(1.0f, 0.0f, 0.0f, 0.7f);
					break;
				default:
					glColor4f(0.0f, 0.0f, 1.0f, 0.7f);
			}

			if (w->HaveTarget()) {
				float3 pos = w->GetCurrentTargetPos();
				float3 v1 = (pos - camera->GetPos()).ANormalize();
				float3 v2 = (v1.cross(UpVector)).ANormalize();
				float3 v3 = (v2.cross(v1)).Normalize();
				float radius = 10.0f;

				if (w->GetCurrentTarget().type == Target_Unit)
					radius = w->GetCurrentTarget().unit->radius;

				// draw the reticle
				glBegin(GL_LINE_STRIP);
				for (int b = 0; b <= 80; ++b) {
					glVertexf3(pos + (v2 * fastmath::sin(b * 2 * PI / 80) + v3 * fastmath::cos(b * 2 * PI / 80)) * radius);
				}
				glEnd();

				if (!w->onlyForward) {
					const CPlayer* p = w->owner->fpsControlPlayer;
					const FPSUnitController& c = p->fpsController;
					const float dist = std::min(c.targetDist, w->range * 0.9f);

					pos = w->aimFromPos + w->wantedDir * dist;
					v1 = (pos - camera->GetPos()).ANormalize();
					v2 = (v1.cross(UpVector)).ANormalize();
					v3 = (v2.cross(v1)).ANormalize();
					radius = dist / 100.0f;

					glBegin(GL_LINE_STRIP);
					for (int b = 0; b <= 80; ++b) {
						glVertexf3(pos + (v2 * fastmath::sin(b * 2 * PI / 80) + v3 * fastmath::cos(b * 2 * PI / 80)) * radius);
					}
					glEnd();
				}

				glBegin(GL_LINES);
				if (!w->onlyForward) {
					glVertexf3(pos);
					glVertexf3(w->GetCurrentTargetPos());

					glVertexf3(pos + (v2 * fastmath::sin(PI * 0.25f) + v3 * fastmath::cos(PI * 0.25f)) * radius);
					glVertexf3(pos + (v2 * fastmath::sin(PI * 1.25f) + v3 * fastmath::cos(PI * 1.25f)) * radius);

					glVertexf3(pos + (v2 * fastmath::sin(PI * -0.25f) + v3 * fastmath::cos(PI * -0.25f)) * radius);
					glVertexf3(pos + (v2 * fastmath::sin(PI * -1.25f) + v3 * fastmath::cos(PI * -1.25f)) * radius);
				}
				if ((w->GetCurrentTargetPos() - camera->GetPos()).ANormalize().dot(camera->GetDir()) < 0.7f) {
					glVertexf3(w->GetCurrentTargetPos());
					glVertexf3(camera->GetPos() + camera->GetDir() * 100.0f);
				}
				glEnd();
			}
		}

	glPopMatrix();
}

void HUDDrawer::Draw(const CUnit* unit)
{
	if (unit == NULL || !draw) {
		return;
	}

	PushState();
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glPushMatrix();
			DrawUnitDirectionArrow(unit);
			DrawCameraDirectionArrow(unit);
		glPopMatrix();

		glEnable(GL_DEPTH_TEST);
		DrawModel(unit);

		DrawWeaponStates(unit);
		DrawTargetReticle(unit);
	PopState();
}
