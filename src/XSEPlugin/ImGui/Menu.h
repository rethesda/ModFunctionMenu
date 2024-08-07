#pragma once

#include <XSEPlugin/Core.h>
#include <XSEPlugin/Util/Singleton.h>

namespace ImGui
{
    class Menu final : public Singleton<Menu>
    {
        friend class Singleton<Menu>;

    public:
        [[nodiscard]] bool IsOpen() const noexcept { return _isOpen.load(); }

        void Open();
        void Close();

        void Draw();

    private:
        Menu() = default;

        ~Menu() = default;

        void DrawExplorer(Datastore* datastore);
        void DrawMessageBox(Datastore* datastore);

        void OnClickParentEntry(MFM_Tree* a_tree);
        void OnClickEntry(MFM_Tree* a_tree, const MFM_Node* a_node);

        void InvokeFunction(const MFM_Function& a_func);

        std::atomic<bool> _isOpen{ false };

        std::vector<char> _msg;
    };
}
