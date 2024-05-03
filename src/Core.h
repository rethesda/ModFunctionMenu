#pragma once

#include <PCH.h>

#include "Function.h"

struct MFM_Function
{
    static MFM_Function Get(const std::filesystem::path& a_path);

    void operator()();
    void operator()(char* a_msg, size_t a_len);

    std::string       dll;
    std::string       api;
    MFMAPI_Type       type{ MFMAPI_Type::kVoid };
    MFMAPI_PreAction  preAction{ MFMAPI_PreAction::kNone };
    MFMAPI_PostAction postAction{ MFMAPI_PostAction::kNone };
};

class MFM_Node
{
public:
    enum class Type : std::uint32_t
    {
        kRegular = 0,
        kDirectory = 1,
    };

    MFM_Node(std::filesystem::path a_path, Type a_type);

    friend bool operator==(const MFM_Node& lhs, const MFM_Node& rhs) noexcept { return lhs.path == rhs.path; }

    friend std::strong_ordering operator<=>(const MFM_Node& lhs, const MFM_Node& rhs) noexcept
    {
        return lhs.path <=> rhs.path;
    }

private:
    void BuildChildren();

public:
    std::filesystem::path path;
    std::string           name;
    Type                  type;
    std::vector<MFM_Node> children;
};

class MFM_Tree
{
public:
    explicit MFM_Tree(std::filesystem::path a_root) : root(std::move(a_root), MFM_Node::Type::kDirectory)
    {
        ResetCurrentRoot();
    }

    const MFM_Node* CurrentRoot() const noexcept { return currentRoot; }
    void            CurrentRoot(const MFM_Node& a_root) { CurrentRoot(std::addressof(a_root)); }
    void            CurrentRoot(const MFM_Node* a_root)
    {
        currentRoot = a_root;
        currentRootStr = PathToStr(a_root->path).substr(39);
    }

    const std::string& CurrentRootStr() const noexcept { return currentRootStr; }

    void ResetCurrentRoot() { CurrentRoot(root); }

private:
    const MFM_Node  root;
    const MFM_Node* currentRoot;
    std::string     currentRootStr;
};
