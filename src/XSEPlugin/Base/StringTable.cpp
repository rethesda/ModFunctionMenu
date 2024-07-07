#include "StringTable.h"

#include <XSEPlugin/Base/Translation.h>

void StringTable::Init()
{
    auto lock = Translation::LockShared();
    auto trans = Translation::GetSingleton();

    Title = trans->Lookup("$Title"sv);
    Section_Mod = trans->Lookup("$Section_Mod"sv);
    Section_Config = trans->Lookup("$Section_Config"sv);
}
