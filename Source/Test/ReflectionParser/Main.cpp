#include <clang-c/Index.h>
#include <iostream>
#include <cassert>

class A
{
public:
    A() = default;
    virtual ~A() = default;
};

class B : public A
{
public:
    B() = default;
    virtual ~B() = default;
};

template <typename T>
size_t getTypeHash()
{
    return typeid(T).hash_code();
}

template <typename T>
size_t getTypeHash(T* object)
{
    return typeid(*object).hash_code();
}

enum class TestEnum
{
    One,
    Two,
    Three,
};

int main()
{
    std::cout << typeid(int).name() << std::endl;
    std::cout << typeid(int*).name() << std::endl;
    std::cout << typeid(TestEnum).name() << std::endl;
    /*CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit unit = clang_parseTranslationUnit(
        index,
        R"(C:\Users\Hypo\Documents\Code\VSCode\test.cpp)",
        nullptr, 0,
        nullptr, 0,
        CXTranslationUnit_None
    );

    assert(unit != nullptr);

    CXCursor cursor = clang_getTranslationUnitCursor(unit);
    clang_visitChildren(
        cursor,
        [](CXCursor cursor, CXCursor parent, CXClientData clientData) -> CXChildVisitResult
        {
            CXString cursorDisplayName = clang_getCursorDisplayName(cursor);
            std::cout << "Current Cursor: " << clang_getCString(cursorDisplayName) << std::endl;

            CXType type = clang_getCursorType(cursor);
            if (type.kind == CXType_Invalid && cursor.kind == CXCursor_AnnotateAttr)
            {
                CXString parentDisplayName = clang_getCursorDisplayName(parent);
                std::cout << clang_getCString(parentDisplayName) << "'s AnnotateAttr is " << clang_getCString(cursorDisplayName) << std::endl;
                clang_disposeString(parentDisplayName);
            }

            CXString typeKindSpelling = clang_getTypeKindSpelling(type.kind);
            std::cout << "Type Kind: " << clang_getCString(typeKindSpelling) << std::endl;
            clang_disposeString(typeKindSpelling);

            if (type.kind == CXType_Pointer ||
                type.kind == CXType_LValueReference ||
                type.kind == CXType_RValueReference)
            {
                CXType pointedType = clang_getPointeeType(type);
                CXString pointedTypeSpelling = clang_getTypeSpelling(pointedType);
                std::cout << "Pointing to type: " << clang_getCString(pointedTypeSpelling) << std::endl;
                clang_disposeString(pointedTypeSpelling);
            }
            else if (type.kind == CXType_Record)
            {
                CXString typeSpelling = clang_getTypeSpelling(type);
                std::cout << clang_getCString(typeSpelling) << std::endl;
                clang_disposeString(typeSpelling);
            }
            std::cout << "\n";

            clang_disposeString(cursorDisplayName);
            return CXChildVisit_Recurse;
        },
        nullptr
    );*/
}
