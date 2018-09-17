/* MollenOS
 *
 * Copyright 2011 - 2018, Philip Meulengracht
 *
 * This program is free software : you can redistribute it and / or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation ? , either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * MollenOS - Vioarr Window Compositor System
 *  - The window compositor system and general window manager for
 *    MollenOS.
 */

#include "../engine/veightengine.hpp"
#include "../engine/elements/sprite.hpp"
#include "../engine/elements/accessbar.hpp"
#include "../engine/scene.hpp"
#include "vioarr.hpp"

CScene *VioarrCompositor::CreateDesktopScene()
{
    CAccessBar *AccessBar   = new CAccessBar(sEngine.GetContext(), _Display->GetWidth(), _Display->GetHeight());
    CSprite *Background     = new CSprite(AccessBar, sEngine.GetContext(), "$sys/themes/default/logo.png", 512, 128);
    Background->Move(sEngine.GetScreenCenterX() - 256.0f, sEngine.GetScreenCenterY() - 64.0f, 0.0f);
    return new CScene(AccessBar);
}
