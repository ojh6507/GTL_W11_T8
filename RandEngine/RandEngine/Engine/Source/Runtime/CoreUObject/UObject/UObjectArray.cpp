#include "UObjectArray.h"

#include "Class.h"
#include "Object.h"
#include "UObjectHash.h"


void FUObjectArray::AddObject(UObject* Object)
{
    ObjObjects.Add(Object);
    AddToClassMap(Object);
}

void FUObjectArray::MarkRemoveObject(UObject* Object)
{
    ObjObjects.Remove(Object);
    RemoveFromClassMap(Object);  // UObjectHashTable에서 Object를 제외
    PendingDestroyObjects.AddUnique(Object);
}

void FUObjectArray::ProcessPendingDestroyObjects()
{
    for (UObject* Object : PendingDestroyObjects)
    {
        const UClass* Class = Object->GetClass();
        std::string ObjectName = Object->GetName().ToAnsiString();
        const uint32 ObjectSize = Class->GetClassSize();

        std::destroy_at(Object);
        FPlatformMemory::AlignedFree<EAT_Object>(Object, ObjectSize);

        UE_LOG(ELogLevel::Display, "Deleted Object: %s, Size: %d", ObjectName, ObjectSize);
    }
    PendingDestroyObjects.Empty();
}

FUObjectArray GUObjectArray;
