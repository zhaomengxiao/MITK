/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

// semantic relations plugin
#include "QmitkDataNodeInformationTypeAction.h"

// semantic relations module
#include <mitkSemanticRelationException.h>

// mitk gui common plugin
#include <mitkDataNodeSelection.h>

// berry
#include <berryISelectionService.h>
#include <berryIWorkbenchPage.h>

// qt
#include <QInputDialog>

QmitkDataNodeInformationTypeAction::QmitkDataNodeInformationTypeAction(QWidget* parent, berry::IWorkbenchPartSite::Pointer workbenchPartSite)
  : QAction(parent)
  , m_WorkbenchPartSite(workbenchPartSite)
{
  setText(tr("Set information type"));
  m_Parent = parent;
  InitializeAction();
}

QmitkDataNodeInformationTypeAction::QmitkDataNodeInformationTypeAction(QWidget* parent, berry::IWorkbenchPartSite* workbenchPartSite)
  : QAction(parent)
  , m_WorkbenchPartSite(berry::IWorkbenchPartSite::Pointer(workbenchPartSite))
{
  setText(tr("Set information type"));
  m_Parent = parent;
  InitializeAction();
}

QmitkDataNodeInformationTypeAction::~QmitkDataNodeInformationTypeAction()
{
  // nothing here
}

void QmitkDataNodeInformationTypeAction::SetDataStorage(mitk::DataStorage* dataStorage)
{
  if (m_DataStorage != dataStorage)
  {
    // set the new data storage
    m_DataStorage = dataStorage;
    m_SemanticRelations = std::make_unique<mitk::SemanticRelations>(m_DataStorage.Lock());
  }
}

void QmitkDataNodeInformationTypeAction::InitializeAction()
{
  connect(this, &QAction::triggered, this, &QmitkDataNodeInformationTypeAction::OnActionTriggered);
}

void QmitkDataNodeInformationTypeAction::OnActionTriggered(bool checked)
{
  if (nullptr == m_SemanticRelations)
  {
    return;
  }

  auto dataNode = GetSelectedNode();
  if (dataNode.IsNull())
  {
    return;
  }

  QInputDialog* inputDialog = new QInputDialog(m_Parent);
  inputDialog->setWindowTitle(tr("Set information type of selected node"));
  inputDialog->setLabelText(tr("Information type:"));
  inputDialog->setTextValue(QString::fromStdString(m_SemanticRelations->GetInformationTypeOfImage(dataNode)));
  inputDialog->setMinimumSize(250, 100);

  int dialogReturnValue = inputDialog->exec();
  if (QDialog::Rejected == dialogReturnValue)
  {
    return;
  }

  try
  {
    m_SemanticRelations->SetInformationType(dataNode, inputDialog->textValue().toStdString());
  }
  catch (const mitk::SemanticRelationException&)
  {
    return;
  }
}

QList<mitk::DataNode::Pointer> QmitkDataNodeInformationTypeAction::GetSelectedNodes()
{
  QList<mitk::DataNode::Pointer> selectedNodes;
  if (m_WorkbenchPartSite.Expired())
  {
    return selectedNodes;
  }

  berry::ISelection::ConstPointer selection = m_WorkbenchPartSite.Lock()->GetWorkbenchWindow()->GetSelectionService()->GetSelection();
  mitk::DataNodeSelection::ConstPointer currentSelection = selection.Cast<const mitk::DataNodeSelection>();

  if (currentSelection.IsNull() || currentSelection->IsEmpty())
  {
    return selectedNodes;
  }

  selectedNodes = QList<mitk::DataNode::Pointer>::fromStdList(currentSelection->GetSelectedDataNodes());
  return selectedNodes;
}

mitk::DataNode::Pointer QmitkDataNodeInformationTypeAction::GetSelectedNode()
{
  QList<mitk::DataNode::Pointer> selectedNodes = GetSelectedNodes();
  if (selectedNodes.empty())
  {
    return nullptr;
  }

  // no batch action; should only be called with a single node
  mitk::DataNode::Pointer dataNode = selectedNodes.front();
  if (nullptr == dataNode)
  {
    return nullptr;
  }

  return dataNode;
}
