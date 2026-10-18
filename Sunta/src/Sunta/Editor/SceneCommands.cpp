#include "Core/SuntaPreCompiled.h"
#include "SceneCommands.h"

#include "SceneDocument.h"
#include "UnsavedChangesDialog.h"

namespace Sunta
{

void SceneCommands::New()
{
	UnsavedChangesDialog::RunAfterAsking([]() { SceneDocument::New(); });
}

void SceneCommands::Open()
{
	UnsavedChangesDialog::RunAfterAsking([]() { SceneDocument::OpenViaDialog(); });
}

void SceneCommands::Save()
{
	SceneDocument::Save();
}

void SceneCommands::SaveAs()
{
	SceneDocument::SaveAs();
}

}
