/***********************************************************************************************
 *                                                                                             *
 * This file is part of the OPL PC Tools project, the graphical PC tools for Open PS2 Loader.  *
 *                                                                                             *
 * OPL PC Tools is free software: you can redistribute it and/or modify it under the terms of  *
 * the GNU General Public License as published by the Free Software Foundation,                *
 * either version 3 of the License, or (at your option) any later version.                     *
 *                                                                                             *
 * OPL PC Tools is distributed in the hope that it will be useful,  but WITHOUT ANY WARRANTY;  *
 * without even the implied warranty of  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  *
 * See the GNU General Public License for more details.                                        *
 *                                                                                             *
 * You should have received a copy of the GNU General Public License along with MailUnit.      *
 * If not, see <http://www.gnu.org/licenses/>.                                                 *
 *                                                                                             *
 ***********************************************************************************************/

#ifndef __OPLPCTOOLS_UICONTEXT__
#define __OPLPCTOOLS_UICONTEXT__

#include <QWidget>
#include <OplPcTools/Core/GameCollection.h>
#include <OplPcTools/Core/GameArtManager.h>

namespace OplPcTools {
namespace UI {

class UIContext
{
public:
    virtual ~UIContext() { }
    virtual void showGameInstaller() = 0;
    virtual void showIsoRecoverer() = 0;
    virtual void showGameDetails(const QString & _id, OplPcTools::Core::GameArtManager & _art_manager) = 0;
    virtual Core::GameCollection & collection() const = 0;
};

} // namespace UI
} // namespace OplPcTools

#endif // __OPLPCTOOLS_UICONTEXT__
