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


#define MOD_ICON_SIZE 150


static bool _modname_cmp(const ModIcon *a, const ModIcon *b)
{
	return a->fname < b->fname;
}

ModSelectorScreen::ModSelectorScreen() : Quad(), currentPanel(-1)
{
	followCamera = 1;
	shareAlphaWithChildren = false;
	alpha = 1;
	alphaMod = 0.1f;
	color = 0;
}

void ModSelectorScreen::onUpdate(float dt)
{
	Quad::onUpdate(dt);

	// mouse wheel scroll
	if(dsq->mouse.scrollWheelChange)
	{
		IconGridPanel *grid = panels[currentPanel];
		InterpolatedVector& v = grid->position;
		float ch = dsq->mouse.scrollWheelChange * 42;
		if(v.isInterpolating())
		{
			v.data->from = v;
			v.data->target.y += ch;
			v.data->timePassed = 0;
		}
		else
		{
			Vector v2 = grid->position;
			v2.y += ch;
			grid->position.interpolateTo(v2, 0.2f, 0, false, true);
		}
		//grid->position = Vector(grid->position.x, grid->position.y + (dsq->mouse.scrollWheelChange * 32));
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
	newgrid->fade(true, true);

	currentPanel = id;

	updateFade();
}

void ModSelectorScreen::updateFade()
{
	// fade out background panels
	// necessary to do all of them, that icon alphas are 0... they would trigger otherwise, even if invisible because parent panel is not shown
	for(int i = 0; i < panels.size(); ++i)
		if(i != currentPanel)
			panels[i]->fade(false, true);
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

	// NEW GRID VIEW

	initModAndPatchPanel();
	// net panel inited on demand

	showPanel(0);

	// we abuse those
	dsq->subtext->alpha = 1.2f;
	dsq->subbox->alpha = 1;

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
	// TODO

	updateFade();
}

static void _ShareAllAlpha(RenderObject *r)
{
	r->shareAlphaWithChildren = true;
	for(RenderObject::Children::iterator it = r->children.begin(); it != r->children.end(); ++it)
		_ShareAllAlpha(*it);
}

void ModSelectorScreen::close()
{
	for(int i = 0; i < panels.size(); ++i)
		if(i != currentPanel)
			panels[i]->setHidden(true);

	const float t = 0.5f;
	_ShareAllAlpha(this);
	//panels[currentPanel]->scale.interpolateTo(Vector(0.9f, 0.9f), t); // HMM
	dsq->subtext->alpha.interpolateTo(0, t/1.2f);
	dsq->subbox->alpha.interpolateTo(0, t);
	dsq->user.save();
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

BasicIcon::BasicIcon() : AquariaGuiQuad(), mouseDown(false),
scaleNormal(1,1), scaleBig(scaleNormal * 1.1f)
{
}

void BasicIcon::onUpdate(float dt)
{
	AquariaGuiQuad::onUpdate(dt);

	if (isCoordinateInside(core->mouse.position))
	{
		scale.interpolateTo(scaleBig, 0.1f);
		const bool anyButton = core->mouse.buttons.left || core->mouse.buttons.right;
		if (anyButton && !mouseDown)
		{
			mouseDown = true;
		}
		else if (!anyButton && mouseDown)
		{
			if(alpha.x > 0.1f) // do not trigger if invis
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

	if (alpha.x > 0.1f && isCoordinateInside(core->mouse.position))
		dsq->subtext->setText(label);
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
				dsq->screenMessage(modname + " - deactivated"); // DEBUG
			}
			else
			{
				dsq->sound->playSfx("pet-on");
				dsq->applyPatch(fname);
				dsq->screenMessage(modname + " - activated"); // DEBUG
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

	setTexture(texToLoad);
	width = height = MOD_ICON_SIZE;

	TiXmlDocument d;

	dsq->mod.loadModXML(&d, entry.path);

	label = "No Description";

	TiXmlElement *top = d.FirstChildElement("AquariaMod");
	if (top)
	{
		TiXmlElement *desc = top->FirstChildElement("Description");
		if (desc)
		{
			if (desc->Attribute("text"))
			{
				label = desc->Attribute("text");
				if (label.size() > 255)
					label.resize(255);
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
	updateStatus();
}

void ModIcon::updateStatus()
{
	if(modType == MODTYPE_PATCH)
	{
		if(dsq->isPatchActive(fname))
		{
			// enabled
			color.interpolateTo(Vector(1,1,1), 0.1f);
			alpha.interpolateTo(1, 0.2f);
			scaleNormal = Vector(1,1);
		}
		else
		{
			// disabled
			color.interpolateTo(Vector(0.5f, 0.5f, 0.5f), 0.1f);
			alpha.interpolateTo(0.6f, 0.2f);
			scaleNormal = Vector(0.8f,0.8f);
		}
		scaleBig = scaleNormal * 1.1f;
	}
}

MenuBasicBar::MenuBasicBar() : AquariaGuiQuad()
{
	setTexture("modselect/bar");
	repeatTextureToFill(true);
	shareAlphaWithChildren = true;
}

void MenuBasicBar::init()
{
}

void MenuIconBar::init()
{
	MenuIcon *ico;
	int y = -height / 2;

	ico = new MenuIcon(0);
	ico->label = "Browse installed mods";
	ico->setTexture("modselect/hdd");
	y += ico->height;
	ico->position = Vector(0, y);
	add(ico);

	ico = new MenuIcon(1);
	ico->label = "Browse & enable/disable installed patches";
	ico->setTexture("modselect/patch");
	y += ico->height;
	ico->position = Vector(0, y);
	add(ico);

	ico = new MenuIcon(2);
	ico->label = "Browse mods online";
	ico->setTexture("modselect/globe");
	y += ico->height;
	ico->position = Vector(0, y);
	add(ico);

	ico = new MenuIcon(3);
	ico->label = "Return to title";
	ico->setTexture("gui/wok-drop");
	ico->repeatTextureToFill(false);
	y += ico->height;
	ico->position = Vector(0, y);
	add(ico);
}

void MenuIconBar::add(MenuIcon *ico)
{
	ico->width = ico->height = width;
	ico->followCamera = 1;
	icons.push_back(ico);
	addChild(ico, PM_POINTER);
}

void MenuArrowBar::init()
{
	// TODO: up/down arrow
}

IconGridPanel::IconGridPanel() : AquariaGuiQuad(), spacing(0), y(0), x(0)
{
	shareAlphaWithChildren = false; // patch selection icons need their own alpha, use fade() instead
	alphaMod = 0.01f;
	color = 0;
}

void IconGridPanel::add(RenderObject *obj)
{
	const int xoffs = (-width / 2) + (obj->width / 2) + spacing;
	const int yoffs = (-height / 2) + obj->height + spacing;
	const int xlim = width - obj->width;
	Vector newpos;

	if(x >= xlim)
	{
		x = 0;
		y += (obj->height + spacing);
	}

	newpos = Vector(x + xoffs, y + yoffs);
	x += (obj->width + spacing);

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

