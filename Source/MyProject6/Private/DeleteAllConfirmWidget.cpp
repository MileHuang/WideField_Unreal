#include "DeleteAllConfirmWidget.h"
#include "AssembleLevelManager.h"

void UDeleteAllConfirmWidget::ConfirmDeleteAll()
{
    if (AssembleManager)
    {
        AssembleManager->DeleteAllParts();
    }

    RemoveFromParent();
}

void UDeleteAllConfirmWidget::CancelDeleteAll()
{
    if (AssembleManager)
    {
        AssembleManager->CloseDeleteAllConfirm();
    }
    else
    {
        RemoveFromParent();
    }
}