/*
Copyright (C) 2007, 2010 - Bit-Blot

This file is part of Aquaria.

Aquaria is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "../BBGE/DebugFont.h"

#include "DSQ.h"
#include "AquariaProgressBar.h"
#include "tinyxml.h"
#include "ModSelector.h"

#ifdef BBGE_BUILD_VFS
#include "ModDownloader.h"
#endif

#define MOD_ICON_SIZE 150
#define MINI_ICON_SIZE 32


static bool _modname_cmp(const ModIcon *a, const ModIcon *b)
{
	return a->fname < b->fname;
}

ModSelectorScreen::ModSelectorScreen() : Quad(), ActionMapper(),
currentPanel(-1), gotServerList(false), dlText(&dsq->smallFont), subtext(&dsq->subsFont)
{
	followCamera = 1;
	shareAlphaWithChildren = false;
	alpha = 1;
	alphaMod = 0.1f;
	color = 0;
	globeIcon = NULL;
	modsIcon = NULL;
	subFadeT = -1;
}

void ModSelectorScreen::moveUp()
{
	move(5);
}

void ModSelectorScreen::moveDown()
{
	move(-5);
}

void ModSelectorScreen::move(int ud, bool instant /* = false */)
{
	IconGridPanel *grid = panels[currentPanel];
	InterpolatedVector& v = grid->position;
	const float ch = ud * 42;
	const float t = instant ? 0.0f : 0.2f;
	if(!instant && v.isInterpolating())
	{
		v.data->from = v;
		v.data->target.y += ch;
		v.data->timePassed = 0;

		if(v.data->target.y > 150)
			v.data->target.y = 150;
		else if(v.data->target.y < -grid->getUsedY() / 2)
			v.data->target.y = -grid->getUsedY() / 2;
	}
	else
	{
		Vector v2 = grid->position;
		v2.y += ch; // scroll down == grid pos y gets negative (grid scrolls up)

		if(v2.y > 150)
			grid->position.interpolateTo(Vector(v2.x, 150), t);
		else if(v2.y < -grid->getUsedY() / 2)
			grid->position.interpolateTo(Vector(v2.x, -grid->getUsedY() / 2), t);
		else
			grid->position.interpolateTo(v2, t, 0, false, true);
	}
}

void ModSelectorScreen::onUpdate(float dt)
{
	Quad::onUpdate(dt);

	// mouse wheel scroll
	if(dsq->mouse.scrollWheelChange)
	{
		move(dsq->mouse.scrollWheelChange);
	}

	if(subFadeT >= 0)
	{
		subFadeT = subFadeT - dt;
		if(subFadeT <= 0)
		{
			subbox.alpha.interpolateTo(0, 1.0f);
			subtext.alpha.interpolateTo(0, 1.2f);
		}
	}

	if(!AquariaGuiElement::currentFocus)
	{
		AquariaGuiElement *closest = AquariaGuiElement::getClosestGuiElement(core->mouse.position);
		if(closest)
		{
			debugLog("Lost focus, setting nearest gui element");
			closest->setFocus(true);
		}
	}
}

void ModSelectorScreen::showPanel(int id)
{
	if(id == currentPanel)
		return;

	const float t = 0.2f;
	IconGridPanel *newgrid = panels[id];

	// fade in selected panel
	if(currentPanel < 0) // just bringing up?
	{
		newgrid->scale = Vector(0.8f,0.8f);
		newgrid->alpha = 0;
	}

	currentPanel = id;

	updateFade();
}

void ModSelectorScreen::updateFade()
{
	// fade out background panels
	// necessary to do all of them, that icon alphas are 0... they would trigger otherwise, even if invisible because parent panel is not shown
	for(int i = 0; i < panels.size(); ++i)
		panels[i]->fade(i == currentPanel, true);
}

static void _MenuIconClickCallback(int id, void *user)
{
	ModSelectorScreen *ms = (ModSelectorScreen*)user;
	switch(id) // see MenuIconBar::init()
	{
		case 2: // network
			ms->initNetPanel();
			break;

		case 3: // exit
			dsq->quitNestedMain();
			return;
	}

	ms->showPanel(id);
}

// can be called multiple times without causing trouble
void ModSelectorScreen::init()
{
	leftbar.width = 100;
	leftbar.height = height;
	leftbar.alpha = 0;
	leftbar.alpha.interpolateTo(1, 0.2f);
	leftbar.position = Vector((leftbar.width - width) / 2, 0);
	leftbar.followCamera = 1;
	if(!leftbar.getParent())
	{
		leftbar.init();
		addChild(&leftbar, PM_STATIC);

		panels.resize(leftbar.icons.size());
		std::fill(panels.begin(), panels.end(), (IconGridPanel*)NULL);
	}

	rightbar.width = 100;
	rightbar.height = height;
	rightbar.alpha = 0;
	rightbar.alpha.interpolateTo(1, 0.2f);
	rightbar.position = Vector(((width - rightbar.width) / 2), 0);
	rightbar.followCamera = 1;
	if(!rightbar.getParent())
	{
		rightbar.init();
		addChild(&rightbar, PM_STATIC);
	}

	for(int i = 0; i < panels.size(); ++i)
	{
		if(panels[i])
			continue;
		panels[i] = new IconGridPanel();
		panels[i]->followCamera = 1;
		panels[i]->width = width - leftbar.width - rightbar.width;
		panels[i]->height = 750;
		panels[i]->position = Vector(0, 0);
		panels[i]->alpha = 0;
		panels[i]->spacing = 20; // for the grid
		panels[i]->scale = Vector(0.8f, 0.8f);
		leftbar.icons[i]->cb = _MenuIconClickCallback;
		leftbar.icons[i]->cb_data = this;
		addChild(panels[i], PM_POINTER);
	}

	arrowUp.useQuad("Gui/arrow-left");
	arrowUp.useSound("click");
	arrowUp.useGlow("particles/glow", 128, 64);
	arrowUp.position = Vector(0, -230);
	arrowUp.followCamera = 1;
	arrowUp.rotation.z = 90;
	arrowUp.event.set(MakeFunctionEvent(ModSelectorScreen, moveUp));
	arrowUp.guiInputLevel = 100;
	arrowUp.alpha = 0;
	arrowUp.alpha.interpolateTo(1, 0.2f);
	arrowUp.setDirMove(DIR_DOWN, &arrowDown);
	rightbar.addChild(&arrowUp, PM_STATIC);

	arrowDown.useQuad("Gui/arrow-right");
	arrowDown.useSound("click");
	arrowDown.useGlow("particles/glow", 128, 64);
	arrowDown.position = Vector(0, 170);
	arrowDown.followCamera = 1;
	arrowDown.rotation.z = 90;
	arrowDown.event.set(MakeFunctionEvent(ModSelectorScreen, moveDown));
	arrowDown.guiInputLevel = 100;
	arrowDown.alpha = 0;
	arrowDown.alpha.interpolateTo(1, 0.2f);
	arrowDown.setDirMove(DIR_UP, &arrowUp);
	rightbar.addChild(&arrowDown, PM_STATIC);

	dlText.alpha = 0;
	dlText.position = Vector(0, 0);
	dlText.setFontSize(15);
	dlText.scale = Vector(1.5f, 1.5f);
	dlText.followCamera = 1;
	addChild(&dlText, PM_STATIC);

	initModAndPatchPanel();
	// net panel inited on demand

	showPanel(0);

	subbox.position = Vector(0,260);
	subbox.alpha = 0;
	subbox.alphaMod = 0.7;
	subbox.followCamera = 1;
	subbox.autoWidth = AUTO_VIRTUALWIDTH;
	subbox.setHeight(80);
	subbox.color = Vector(0, 0, 0);
	addChild(&subbox, PM_STATIC);

	subtext.position = Vector(0,230);
	subtext.followCamera = 1;
	subtext.alpha = 0;
	subtext.setFontSize(12);
	subtext.setWidth(800);
	subtext.setAlign(ALIGN_CENTER);
	addChild(&subtext, PM_STATIC);

	dsq->toggleVersionLabel(false);

	modsIcon->setFocus(true);

	// TODO: keyboard/gamepad control
}

void ModSelectorScreen::initModAndPatchPanel()
{
	IconGridPanel *modgrid = panels[0];
	IconGridPanel *patchgrid = panels[1];
	ModIcon *ico;
	std::vector<ModIcon*> tv; // for sorting
	tv.resize(dsq->modEntries.size());
	for(unsigned int i = 0; i < tv.size(); ++i)
	{
		ico = NULL;
		for(RenderObject::Children::iterator it = modgrid->children.begin(); it != modgrid->children.end(); ++it)
			if(ModIcon* other = dynamic_cast<ModIcon*>(*it))
				if(other->modId == i)
				{
					ico = other;
					break;
				}

		if(!ico)
		{
			for(RenderObject::Children::iterator it = patchgrid->children.begin(); it != patchgrid->children.end(); ++it)
				if(ModIcon* other = dynamic_cast<ModIcon*>(*it))
					if(other->modId == i)
					{
						ico = other;
						break;
					}

			if(!ico) // ok, its really not there.
			{
				ico = new ModIcon;
				ico->followCamera = 1;
				std::ostringstream os;
				os << "Created ModIcon " << i;
				debugLog(os.str());
			}
		}

		tv[i] = ico;
		ico->loadEntry(dsq->modEntries[i]);
	}
	std::sort(tv.begin(), tv.end(), _modname_cmp);

	for(int i = 0; i < tv.size(); ++i)
	{
		if(!tv[i]->getParent()) // ensure it was not added earlier
		{
			if(tv[i]->modType == MODTYPE_PATCH)
				patchgrid->add(tv[i]);
			else
				modgrid->add(tv[i]);
		}
	}
	updateFade();
}

void ModSelectorScreen::initNetPanel()
{
#ifdef BBGE_BUILD_VFS
	if(!gotServerList)
	{
		// FIXME: demo should be able to see downloadable mods imho
#ifndef AQUARIA_DEMO
		moddl.init();
		std::string serv = dsq->user.network.masterServer;
		if(serv.empty())
			serv = DEFAULT_MASTER_SERVER;
		moddl.GetModlist(serv, true, true);
#endif
		gotServerList = true; // try this only once (is automatically reset on failure)
	}
#endif
}

void ModSelectorScreen::setSubText(const std::string& s)
{
	subtext.setText(s);
	subtext.alpha.interpolateTo(1, 0.2f);
	subbox.alpha.interpolateTo(1, 0.2f);
	subFadeT = 1;
}

static void _FadeOutAll(RenderObject *r, float t)
{
	//r->shareAlphaWithChildren = true;
	r->alpha.interpolateTo(0, t);
	for(RenderObject::Children::iterator it = r->children.begin(); it != r->children.end(); ++it)
		_FadeOutAll(*it, t);
}

void ModSelectorScreen::close()
{
	/*for(int i = 0; i < panels.size(); ++i)
		if(i != currentPanel)
			panels[i]->setHidden(true);*/

	const float t = 0.5f;
	_FadeOutAll(this, t);
	//panels[currentPanel]->scale.interpolateTo(Vector(0.9f, 0.9f), t); // HMM
	dsq->user.save();
	dsq->toggleVersionLabel(true);

	// kinda hackish
	/*dlText.setHidden(true);
	arrowDown.glow->setHidden(true);
	arrowUp.glow->setHidden(true);
	subbox.setHidden(true);
	subtext.setHidden(true);*/
}

JuicyProgressBar::JuicyProgressBar() : Quad(), txt(&dsq->smallFont)
{
	setTexture("modselect/tube");
	//shareAlphaWithChildren = true;
	followCamera = 1;
	alpha = 1;

	juice.setTexture("loading/juice");
	juice.alpha = 0.8;
	juice.followCamera = 1;
	addChild(&juice, PM_STATIC);

	txt.alpha = 0.7;
	txt.followCamera = 1;
	addChild(&txt, PM_STATIC);

	progress(0);
}

void JuicyProgressBar::progress(float p)
{
	juice.width = p * width;
	juice.height = height - 4;
	perc = p;
}

BasicIcon::BasicIcon()
: mouseDown(false), scaleNormal(1,1), scaleBig(scaleNormal * 1.1f)
{
	// HACK: Because AquariaMenuItem assigns onClick() in it's ctor,
	// but we handle this ourselves.
	clearCreatedEvents();
	clearActions();
	shareAlpha = true;
	guiInputLevel = 100;
}

bool BasicIcon::isGuiVisible()
{
	return !isHidden() && alpha.x > 0.1f && alphaMod > 0.1f && (!parent || parent->alpha.x == 1);
}

bool BasicIcon::isCursorInMenuItem()
{
	if(quad)
		return quad->isCoordinateInside(core->mouse.position);
	return AquariaMenuItem::isCursorInMenuItem();
}

void BasicIcon::onUpdate(float dt)
{
	AquariaMenuItem::onUpdate(dt);

	// Autoscroll if selecting icon outside of screen
	if(hasFocus && dsq->modSelectorScr)
	{
		Vector pos = getRealPosition();
		if(pos.y < 20 || pos.y > 580)
		{
			if(pos.y < 300)
				dsq->modSelectorScr->move(5, true);
			else
				dsq->modSelectorScr->move(-5, true);
			core->main(FRAME_TIME); // HACK: this is necessary to correctly position the mouse on the object after mofing the panel
			setFocus(true); // re-position mouse
		}
	}

	if(!quad)
		return;

	if (hasInput() && quad->isCoordinateInside(core->mouse.position))
	{
		scale.interpolateTo(scaleBig, 0.1f);
		const bool anyButton = core->mouse.buttons.left || core->mouse.buttons.right;
		if (anyButton && !mouseDown)
		{
			mouseDown = true;
		}
		else if (!anyButton && mouseDown)
		{
			if(isGuiVisible()) // do not trigger if invis
				onClick();
			mouseDown = false;
		}
	}
	else
	{
		scale.interpolateTo(scaleNormal, 0.1f);
		mouseDown = false;
	}
}

void SubtitleIcon::onUpdate(float dt)
{
	BasicIcon::onUpdate(dt);

	if (dsq->modSelectorScr && isGuiVisible() && quad && quad->isCoordinateInside(core->mouse.position))
		dsq->modSelectorScr->setSubText(label);
}

void BasicIcon::onClick()
{
	dsq->sound->playSfx("denied");
}

MenuIcon::MenuIcon(int id) : SubtitleIcon(), iconId(id), cb(0), cb_data(0)
{
}

void MenuIcon::onClick()
{
	dsq->sound->playSfx("click");
	if(cb)
		cb(iconId, cb_data);
}


ModIcon::ModIcon(): SubtitleIcon(), modId(-1)
{
}

void ModIcon::onClick()
{
	dsq->sound->playSfx("click");

#ifdef AQUARIA_DEMO
	dsq->nag(NAG_TOTITLE);
	return;
#endif

	switch(modType)
	{
		case MODTYPE_MOD:
		{
			dsq->sound->playSfx("pet-on");
			core->quitNestedMain();
			dsq->modIsSelected = true;
			dsq->selectedMod = modId;
			break;
		}

		case MODTYPE_PATCH:
		{
			std::set<std::string>::iterator it = dsq->activePatches.find(fname);
			if(it != dsq->activePatches.end())
			{
				dsq->sound->playSfx("pet-off");
				dsq->unapplyPatch(fname);
				//dsq->screenMessage(modname + " - deactivated"); // DEBUG
			}
			else
			{
				dsq->sound->playSfx("pet-on");
				dsq->applyPatch(fname);
				//dsq->screenMessage(modname + " - activated"); // DEBUG
			}
			updateStatus();
			break;
		}

		default:
			errorLog("void ModIcon::onClick() -- unknown modType");
	}
}

void ModIcon::loadEntry(const ModEntry& entry)
{
	modId = entry.id;
	modType = entry.type;
	fname = entry.path;

	std::string texToLoad = entry.path + "/" + "mod-icon";

	texToLoad = dsq->mod.getBaseModPath() + texToLoad;

	if(!quad)
		useQuad(texToLoad);
	quad->setWidthHeight(MOD_ICON_SIZE, MOD_ICON_SIZE);

	TiXmlDocument d;

	dsq->mod.loadModXML(&d, entry.path);

	std::string ds = "No Description";

	TiXmlElement *top = d.FirstChildElement("AquariaMod");
	if (top)
	{
		TiXmlElement *desc = top->FirstChildElement("Description");
		if (desc)
		{
			if (desc->Attribute("text"))
			{
				ds = desc->Attribute("text");
				//if (label.size() > 255)
				//	label.resize(255);
			}
		}
		TiXmlElement *fullname = top->FirstChildElement("Fullname");
		if (fullname)
		{
			if (fullname->Attribute("text"))
			{
				modname = fullname->Attribute("text");
				if (modname.size() > 60)
					modname.resize(60);
			}
		}
	}

	label = "--[ " + modname + " ]--\n" + ds;

	updateStatus();
}

void ModIcon::updateStatus()
{
	if(modType == MODTYPE_PATCH)
	{
		if(dsq->isPatchActive(fname))
		{
			// enabled
			quad->color.interpolateTo(Vector(1,1,1), 0.1f);
			alpha.interpolateTo(1, 0.2f);
			scaleNormal = Vector(1,1);
		}
		else
		{
			// disabled
			quad->color.interpolateTo(Vector(0.5f, 0.5f, 0.5f), 0.1f);
			alpha.interpolateTo(0.6f, 0.2f);
			scaleNormal = Vector(0.8f,0.8f);
		}
		scaleBig = scaleNormal * 1.1f;
	}
}


#ifdef BBGE_BUILD_VFS


ModIconOnline::ModIconOnline()
: SubtitleIcon(), pb(0), extraIcon(0), statusIcon(0), clickable(true), isPatch(false), hasUpdate(false)
{
	label = desc;
}

bool ModIconOnline::hasPkgOnDisk()
{
	if(localname.empty())
		return false;
	std::string modfile = dsq->mod.getBaseModPath() + localname + ".aqmod";
	return exists(modfile.c_str(), false, true);
}

// return true if the desired texture could be set
bool ModIconOnline::fixIcon()
{
	bool result = false;
	if(exists(iconfile, false, true))
	{
		if(quad)
		{
			quad->fadeAlphaWithLife = true;
			quad->setLife(1);
			quad->setDecayRate(2);
			quad = 0;
		}
		useQuad(iconfile);
		result = Texture::textureError == TEXERR_OK;
	}
	if(!quad)
	{
		//useQuad("bitblot/logo");
		int i = (rand() % 7) + 1;
		std::stringstream ss;
		ss << "fish-000" << i;
		useQuad(ss.str());
	}

	quad->alpha = 0.001;
	quad->setWidthHeight(MOD_ICON_SIZE, MOD_ICON_SIZE);
	quad->alpha.interpolateTo(1, 0.5f);

	if(!extraIcon && isPatch)
	{
		Vector pos(-MOD_ICON_SIZE/2 + MINI_ICON_SIZE/2, MOD_ICON_SIZE/2 - MINI_ICON_SIZE/2);
		extraIcon = new Quad("modselect/ico_patch", pos);
		extraIcon->setWidthHeight(MINI_ICON_SIZE, MINI_ICON_SIZE);
		quad->addChild(extraIcon, PM_POINTER);
	}

	if(statusIcon)
	{
		statusIcon->fadeAlphaWithLife = true;
		statusIcon->setLife(1);
		statusIcon->setDecayRate(2);
		statusIcon = 0;
	}

	if(!statusIcon)
	{
		Vector pos(MOD_ICON_SIZE/2 - MINI_ICON_SIZE/2, MOD_ICON_SIZE/2 - MINI_ICON_SIZE/2);
		if(dsq->modIsKnown(localname))
		{
			// installed manually?
			if(!hasPkgOnDisk())
			{
				statusIcon = new Quad("modselect/ico_locked", pos);
			}
			else if(hasUpdate)
			{
				statusIcon = new Quad("modselect/ico_update", pos);
				statusIcon->alpha.interpolateTo(0.5f, 0.5f, -1, true, true);
			}
			else 
				statusIcon = new Quad("modselect/ico_check", pos);
		}

		if(statusIcon)
		{
			statusIcon->setWidthHeight(MINI_ICON_SIZE, MINI_ICON_SIZE);
			quad->addChild(statusIcon, PM_POINTER);
		}
	}

	return result;
}

void ModIconOnline::onClick()
{
	dsq->sound->playSfx("click");

#ifdef AQUARIA_DEMO
	dsq->nag(NAG_TOTITLE);
	return;
#endif

	bool success = false;

	if(clickable && !packageUrl.empty())
	{
		bool proceed = true;
		if(dsq->modIsKnown(localname))
		{
			mouseDown = false; // HACK: do this here else stack overflow!
			if(hasPkgOnDisk())
			{
				if(hasUpdate)
					proceed = dsq->confirm("Download update?"); // TODO: -> stringbank
				else
					proceed = dsq->confirm("Mod already exists. Re-download?");
			}
			else
			{
				dsq->confirm("This mod was installed manually,\nnot messing with it.", "", true);
				proceed = false;
			}

		}

		if(proceed && confirmStr.length())
		{
			mouseDown = false; // HACK: do this here else stack overflow!
			dsq->sound->playSfx("spirit-beacon");
			proceed = dsq->confirm(confirmStr);
		}

		if(proceed)
		{
			moddl.GetMod(packageUrl, localname);
			setDownloadProgress(0);
			success = true;
			clickable = false;
		}
		else
			success = true; // we didn't want, anyway
	}

	if(!success)
	{
		SubtitleIcon::onClick(); // denied
	}
}

void ModIconOnline::setDownloadProgress(float p, float barheight /* = 20 */)
{
	if(p >= 0 && p <= 1)
	{
		if(!pb)
		{
			pb = new JuicyProgressBar;
			addChild(pb, PM_POINTER);
			pb->width = quad->width;
			pb->height = 0;
			pb->alpha = 0;
		}

		if(barheight != pb->height)
		{
			pb->height = barheight;
			pb->width = quad->width;
			pb->position = Vector(0, (quad->height - pb->height + 1) / 2); // +1 skips a pixel row and looks better
		}

		pb->alpha.interpolateTo(1, 0.2f);
		pb->progress(p);
	}
	else if(pb)
	{
		pb->fadeAlphaWithLife = true;
		pb->setLife(1);
		pb->setDecayRate(2);
		pb = 0;
	}
}

#endif // BBGE_BUILD_VFS

MenuBasicBar::MenuBasicBar()
{
	setTexture("modselect/bar");
	repeatTextureToFill(true);
	shareAlphaWithChildren = false;
}

void MenuBasicBar::init()
{
}

void MenuIconBar::init()
{
	MenuIcon *ico;
	int y = (-height / 2) - 35;

	
	ico = new MenuIcon(0);
	ico->label = "\nBrowse installed mods"; // TODO: -> stringbank
	ico->useQuad("modselect/hdd");
	y += ico->quad->height;
	ico->position = Vector(0, y);
	add(ico);
	dsq->modSelectorScr->modsIcon = ico; // HACK

	MenuIcon *prev = ico;
	ico = new MenuIcon(1);
	ico->label = "\nBrowse & enable/disable installed patches";
	ico->useQuad("modselect/patch");
	y += ico->quad->height;
	ico->position = Vector(0, y);
	ico->setDirMove(DIR_UP, prev);
	prev->setDirMove(DIR_DOWN, ico);
	add(ico);

	prev = ico;
	ico = new MenuIcon(2);
	ico->label = "\nBrowse mods online";
	ico->useQuad("modselect/globe");
	y += ico->quad->height;
	ico->position = Vector(0, y);
	ico->setDirMove(DIR_UP, prev);
	prev->setDirMove(DIR_DOWN, ico);
	add(ico);
	dsq->modSelectorScr->globeIcon = ico; // HACK

	prev = ico;
	ico = new MenuIcon(3);
	ico->label = "\nReturn to title";
	ico->useQuad("gui/wok-drop");
	ico->repeatTextureToFill(false);
	y += ico->quad->height;
	ico->position = Vector(0, y);
	ico->setDirMove(DIR_UP, prev);
	prev->setDirMove(DIR_DOWN, ico);
	add(ico);
}

void MenuIconBar::add(MenuIcon *ico)
{
	ico->quad->setWidthHeight(width, width);
	ico->followCamera = 1;
	icons.push_back(ico);
	addChild(ico, PM_POINTER);
}

void MenuArrowBar::init()
{
	// TODO: up/down arrow
}

IconGridPanel::IconGridPanel()
: spacing(0), y(0), x(0)
{
	shareAlphaWithChildren = false; // patch selection icons need their own alpha, use fade() instead
	alphaMod = 0.01f;
	color = 0;
}

void IconGridPanel::add(BasicIcon *obj)
{
	const int xoffs = (-width / 2) + (obj->quad->width / 2) + spacing;
	const int yoffs = (-height / 2) + obj->quad->height + spacing;
	const int xlim = width - obj->quad->width;
	Vector newpos;

	if(x >= xlim)
	{
		x = 0;
		y += (obj->quad->height + spacing);
	}

	newpos = Vector(x + xoffs, y + yoffs);
	x += (obj->quad->width + spacing);

	obj->position = newpos;
	addChild(obj, PM_POINTER);
}

void IconGridPanel::fade(bool in, bool sc)
{
	const float t = 0.2f;
	Vector newalpha;
	if(in)
	{
		newalpha.x = 1;
		if(sc)
			scale.interpolateTo(Vector(1, 1), t);
	}
	else
	{
		newalpha.x = 0;
		if(sc)
			scale.interpolateTo(Vector(0.8f, 0.8f), t);
	}
	alpha.interpolateTo(newalpha, t);

	for(Children::iterator it = children.begin(); it != children.end(); ++it)
	{
		(*it)->alpha.interpolateTo(newalpha, t);

		if(in)
			if(ModIcon *ico = dynamic_cast<ModIcon*>(*it))
				ico->updateStatus();
	}
}

