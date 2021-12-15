/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include <mitkContourModelUtils.h>
#include <mitkLiveWireTool2D.h>
#include <mitkLiveWireTool2D.xpm>
#include <mitkToolManager.h>

#include <usGetModuleContext.h>
#include <usModuleResource.h>

#include <type_traits>

namespace mitk
{
  MITK_TOOL_MACRO(MITKSEGMENTATION_EXPORT, LiveWireTool2D, "LiveWire tool");
}

mitk::LiveWireTool2D::LiveWireTool2D()
  : SegTool2D("LiveWireTool"), m_CreateAndUseDynamicCosts(false)
{
}

mitk::LiveWireTool2D::~LiveWireTool2D()
{
  this->ClearSegmentation();
}

void mitk::LiveWireTool2D::RemoveHelperObjects()
{
  auto dataStorage = this->GetToolManager()->GetDataStorage();

  if (nullptr == dataStorage)
    return;

  for (const auto &editingContour : m_EditingContours)
    dataStorage->Remove(editingContour.first);

  for (const auto &workingContour : m_WorkingContours)
    dataStorage->Remove(workingContour.first);

  if (m_EditingContourNode.IsNotNull())
    dataStorage->Remove(m_EditingContourNode);

  if (m_LiveWireContourNode.IsNotNull())
    dataStorage->Remove(m_LiveWireContourNode);

  if (m_ClosureContourNode.IsNotNull())
    dataStorage->Remove(m_ClosureContourNode);

  if (m_ContourNode.IsNotNull())
    dataStorage->Remove(m_ContourNode);

  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void mitk::LiveWireTool2D::ReleaseHelperObjects()
{
  this->RemoveHelperObjects();

  m_EditingContours.clear();
  m_WorkingContours.clear();

  m_EditingContourNode = nullptr;
  m_EditingContour = nullptr;

  m_LiveWireContourNode = nullptr;
  m_LiveWireContour = nullptr;

  m_ClosureContourNode = nullptr;
  m_ClosureContour = nullptr;

  m_ContourNode = nullptr;
  m_Contour = nullptr;
}

void mitk::LiveWireTool2D::ReleaseInteractors()
{
  this->EnableContourLiveWireInteraction(false);
  m_LiveWireInteractors.clear();
}

void mitk::LiveWireTool2D::ConnectActionsAndFunctions()
{
  CONNECT_FUNCTION("InitObject", OnInitLiveWire);
  CONNECT_FUNCTION("AddPoint", OnAddPoint);
  CONNECT_FUNCTION("CtrlAddPoint", OnAddPoint);
  CONNECT_FUNCTION("MovePoint", OnMouseMoveNoDynamicCosts);
  CONNECT_FUNCTION("FinishContour", OnFinish);
  CONNECT_FUNCTION("DeletePoint", OnLastSegmentDelete);
  CONNECT_FUNCTION("CtrlMovePoint", OnMouseMoved);
}

const char **mitk::LiveWireTool2D::GetXPM() const
{
  return mitkLiveWireTool2D_xpm;
}

us::ModuleResource mitk::LiveWireTool2D::GetIconResource() const
{
  return us::GetModuleContext()->GetModule()->GetResource("LiveWire_48x48.png");
}

us::ModuleResource mitk::LiveWireTool2D::GetCursorIconResource() const
{
  return us::GetModuleContext()->GetModule()->GetResource("LiveWire_Cursor_32x32.png");
}

const char *mitk::LiveWireTool2D::GetName() const
{
  return "Live Wire";
}

void mitk::LiveWireTool2D::Activated()
{
  Superclass::Activated();
  this->ResetToStartState();
  this->EnableContourLiveWireInteraction(true);
}

void mitk::LiveWireTool2D::Deactivated()
{
  this->ConfirmSegmentation();
  Superclass::Deactivated();
}

void mitk::LiveWireTool2D::UpdateLiveWireContour()
{
  if (m_Contour.IsNotNull())
  {
    auto timeGeometry = m_Contour->GetTimeGeometry()->Clone();
    m_LiveWireContour = this->m_LiveWireFilter->GetOutput();
    m_LiveWireContour->SetTimeGeometry(timeGeometry); // needed because the results of the filter are always from 0 ms
                                                      // to 1 ms and the filter also resets its outputs.
    m_LiveWireContourNode->SetData(this->m_LiveWireContour);

    m_ClosureContour = this->m_LiveWireFilterClosure->GetOutput();
    m_ClosureContour->SetTimeGeometry(timeGeometry); // needed because the results of the filter are always from 0 ms
                                                     // to 1 ms and the filter also resets its outputs.
    m_ClosureContourNode->SetData(this->m_ClosureContour);
  }
}

void mitk::LiveWireTool2D::OnTimePointChanged()
{
  auto reference = this->GetReferenceData();
  if (nullptr == reference || m_PlaneGeometry.IsNull() || m_LiveWireFilter.IsNull() || m_LiveWireContourNode.IsNull())
    return;

  auto timeStep = reference->GetTimeGeometry()->TimePointToTimeStep(this->GetLastTimePointTriggered());

  m_ReferenceDataSlice = GetAffectedImageSliceAs2DImageByTimePoint(m_PlaneGeometry, reference, timeStep);
  m_LiveWireFilter->SetInput(m_ReferenceDataSlice);

  m_LiveWireFilter->Update();

  m_LiveWireFilterClosure->SetInput(m_ReferenceDataSlice);

  m_LiveWireFilterClosure->Update();

  this->UpdateLiveWireContour();

  RenderingManager::GetInstance()->RequestUpdateAll();
};


void mitk::LiveWireTool2D::EnableContourLiveWireInteraction(bool on)
{
  for (const auto &interactor : m_LiveWireInteractors)
    interactor->EnableInteraction(on);
}

void mitk::LiveWireTool2D::ConfirmSegmentation()
{
  auto referenceImage = this->GetReferenceData();
  auto workingImage = this->GetWorkingData();

  if (nullptr != referenceImage && nullptr != workingImage)
  {
    std::vector<SliceInformation> sliceInfos;
    sliceInfos.reserve(m_WorkingContours.size());

    const auto currentTimePoint = mitk::RenderingManager::GetInstance()->GetTimeNavigationController()->GetSelectedTimePoint();
    TimeStepType workingImageTimeStep = workingImage->GetTimeGeometry()->TimePointToTimeStep(currentTimePoint);

    for (const auto &workingContour : m_WorkingContours)
    {
      auto contour = dynamic_cast<ContourModel *>(workingContour.first->GetData());

      if (nullptr == contour || contour->IsEmpty())
        continue;

      auto sameSlicePredicate = [&workingContour, workingImageTimeStep](const SliceInformation& si) { return workingContour.second->IsOnPlane(si.plane) && workingImageTimeStep == si.timestep; };

      auto finding = std::find_if(sliceInfos.begin(), sliceInfos.end(), sameSlicePredicate);
      if (finding == sliceInfos.end())
      {
        auto workingSlice = this->GetAffectedImageSliceAs2DImage(workingContour.second, workingImage, workingImageTimeStep)->Clone();
        sliceInfos.emplace_back(workingSlice, workingContour.second, workingImageTimeStep);
        finding = std::prev(sliceInfos.end());
      }

      //cast const away is OK in this case, because these are all slices created and manipulated
      //localy in this function call. And we want to keep the high constness of SliceInformation for
      //public interfaces.
      auto workingSlice = const_cast<Image*>(finding->slice.GetPointer());

      auto projectedContour = ContourModelUtils::ProjectContourTo2DSlice(workingSlice, contour);
      int activePixelValue = ContourModelUtils::GetActivePixelValue(workingImage);

      ContourModelUtils::FillContourInSlice(
        projectedContour, workingSlice, workingImage, activePixelValue);
    }

    this->WriteBackSegmentationResults(sliceInfos);
  }

  this->ClearSegmentation();
}

void mitk::LiveWireTool2D::ClearSegmentation()
{
  this->ReleaseHelperObjects();
  this->ReleaseInteractors();
  this->ResetToStartState();
}

bool mitk::LiveWireTool2D::IsPositionEventInsideImageRegion(mitk::InteractionPositionEvent *positionEvent,
                                                            mitk::BaseData *data)
{
  bool isPositionEventInsideImageRegion = nullptr != data && data->GetGeometry()->IsInside(positionEvent->GetPositionInWorld());

  if (!isPositionEventInsideImageRegion)
    MITK_WARN("LiveWireTool2D") << "PositionEvent is outside ImageRegion!";

  return isPositionEventInsideImageRegion;
}

mitk::ContourModel::Pointer mitk::LiveWireTool2D::CreateNewContour() const
{
  auto workingData = this->GetWorkingData();
  if (nullptr == workingData)
  {
    this->InteractiveSegmentationBugMessage("Cannot create new contour. No valid working data is set. Application is in invalid state.");
    mitkThrow() << "Cannot create new contour. No valid working data is set. Application is in invalid state.";
  }

  auto contour = ContourModel::New();

  //generate a time geometry that is always visible as the working contour should always be.
  auto contourTimeGeometry = ProportionalTimeGeometry::New();
  contourTimeGeometry->SetStepDuration(std::numeric_limits<TimePointType>::max());
  contourTimeGeometry->SetTimeStepGeometry(contour->GetTimeGeometry()->GetGeometryForTimeStep(0)->Clone(), 0);
  contour->SetTimeGeometry(contourTimeGeometry);

  return contour;
}

void mitk::LiveWireTool2D::OnInitLiveWire(StateMachineAction *, InteractionEvent *interactionEvent)
{
  auto positionEvent = dynamic_cast<mitk::InteractionPositionEvent *>(interactionEvent);

  if (nullptr == positionEvent)
    return;

  auto workingDataNode = this->GetWorkingDataNode();

  if (!IsPositionEventInsideImageRegion(positionEvent, workingDataNode->GetData()))
  {
    this->ResetToStartState();
    return;
  }

  m_LastEventSender = positionEvent->GetSender();
  m_LastEventSlice = m_LastEventSender->GetSlice();

  m_Contour = this->CreateNewContour();
  m_ContourNode = mitk::DataNode::New();
  m_ContourNode->SetData(m_Contour);
  m_ContourNode->SetName("working contour node");
  m_ContourNode->SetProperty("layer", IntProperty::New(100));
  m_ContourNode->AddProperty("fixedLayer", BoolProperty::New(true));
  m_ContourNode->SetProperty("helper object", mitk::BoolProperty::New(true));
  m_ContourNode->AddProperty("contour.color", ColorProperty::New(1.0f, 1.0f, 0.0f), nullptr, true);
  m_ContourNode->AddProperty("contour.points.color", ColorProperty::New(1.0f, 0.0f, 0.1f), nullptr, true);
  m_ContourNode->AddProperty("contour.controlpoints.show", BoolProperty::New(true), nullptr, true);

  m_LiveWireContour = this->CreateNewContour();
  m_LiveWireContourNode = mitk::DataNode::New();
  m_LiveWireContourNode->SetData(m_LiveWireContour);
  m_LiveWireContourNode->SetName("active livewire node");
  m_LiveWireContourNode->SetProperty("layer", IntProperty::New(101));
  m_LiveWireContourNode->AddProperty("fixedLayer", BoolProperty::New(true));
  m_LiveWireContourNode->SetProperty("helper object", mitk::BoolProperty::New(true));
  m_LiveWireContourNode->AddProperty("contour.color", ColorProperty::New(0.1f, 1.0f, 0.1f), nullptr, true);
  m_LiveWireContourNode->AddProperty("contour.width", mitk::FloatProperty::New(4.0f), nullptr, true);

  m_ClosureContour = this->CreateNewContour();
  m_ClosureContourNode = mitk::DataNode::New();
  m_ClosureContourNode->SetData(m_ClosureContour);
  m_ClosureContourNode->SetName("active closure node");
  m_ClosureContourNode->SetProperty("layer", IntProperty::New(101));
  m_ClosureContourNode->AddProperty("fixedLayer", BoolProperty::New(true));
  m_ClosureContourNode->SetProperty("helper object", mitk::BoolProperty::New(true));
  m_ClosureContourNode->AddProperty("contour.color", ColorProperty::New(0.0f, 1.0f, 0.1f), nullptr, true);
  m_ClosureContourNode->AddProperty("contour.width", mitk::FloatProperty::New(2.0f), nullptr, true);

  m_EditingContour = this->CreateNewContour();
  m_EditingContourNode = mitk::DataNode::New();
  m_EditingContourNode->SetData(m_EditingContour);
  m_EditingContourNode->SetName("editing node");
  m_EditingContourNode->SetProperty("layer", IntProperty::New(102));
  m_EditingContourNode->AddProperty("fixedLayer", BoolProperty::New(true));
  m_EditingContourNode->SetProperty("helper object", mitk::BoolProperty::New(true));
  m_EditingContourNode->AddProperty("contour.color", ColorProperty::New(0.1f, 1.0f, 0.1f), nullptr, true);
  m_EditingContourNode->AddProperty("contour.points.color", ColorProperty::New(0.0f, 0.0f, 1.0f), nullptr, true);
  m_EditingContourNode->AddProperty("contour.width", mitk::FloatProperty::New(4.0f), nullptr, true);

  auto dataStorage = this->GetToolManager()->GetDataStorage();
  dataStorage->Add(m_ContourNode, workingDataNode);
  dataStorage->Add(m_LiveWireContourNode, workingDataNode);
  dataStorage->Add(m_ClosureContourNode, workingDataNode);
  dataStorage->Add(m_EditingContourNode, workingDataNode);

  // Set current slice as input for ImageToLiveWireContourFilter
  m_ReferenceDataSlice = this->GetAffectedReferenceSlice(positionEvent);

  auto origin = m_ReferenceDataSlice->GetSlicedGeometry()->GetOrigin();
  m_ReferenceDataSlice->GetSlicedGeometry()->WorldToIndex(origin, origin);
  m_ReferenceDataSlice->GetSlicedGeometry()->IndexToWorld(origin, origin);
  m_ReferenceDataSlice->GetSlicedGeometry()->SetOrigin(origin);

  m_LiveWireFilter = ImageLiveWireContourModelFilter::New();
  m_LiveWireFilter->SetUseCostFunction(true);
  m_LiveWireFilter->SetInput(m_ReferenceDataSlice);

  m_LiveWireFilterClosure = ImageLiveWireContourModelFilter::New();
  m_LiveWireFilterClosure->SetUseCostFunction(false);
  m_LiveWireFilterClosure->SetInput(m_ReferenceDataSlice);

  // Map click to pixel coordinates
  auto click = positionEvent->GetPositionInWorld();
  itk::Index<3> idx;
  m_ReferenceDataSlice->GetGeometry()->WorldToIndex(click, idx);

  // Get the pixel with the highest gradient in a 7x7 region
  itk::Index<3> indexWithHighestGradient;
  AccessFixedDimensionByItk_2(m_ReferenceDataSlice, FindHighestGradientMagnitudeByITK, 2, idx, indexWithHighestGradient);

  click[0] = indexWithHighestGradient[0];
  click[1] = indexWithHighestGradient[1];
  click[2] = indexWithHighestGradient[2];
  m_ReferenceDataSlice->GetGeometry()->IndexToWorld(click, click);

  // Set initial start point
  m_Contour->AddVertex(click, true);
  m_LiveWireFilter->SetStartPoint(click);
  //m_LiveWireFilterClosure->SetStartPoint(click);
  m_LiveWireFilterClosure->SetEndPoint(click);

  // Remember PlaneGeometry to determine if events were triggered in the same plane
  m_PlaneGeometry = interactionEvent->GetSender()->GetCurrentWorldPlaneGeometry();

  m_CreateAndUseDynamicCosts = true;

  mitk::RenderingManager::GetInstance()->RequestUpdate(positionEvent->GetSender()->GetRenderWindow());
}

void mitk::LiveWireTool2D::OnAddPoint(StateMachineAction *, InteractionEvent *interactionEvent)
{
  // Complete LiveWire interaction for the last segment. Add current LiveWire contour to
  // the finished contour and reset to start a new segment and computation.

  auto positionEvent = dynamic_cast<mitk::InteractionPositionEvent *>(interactionEvent);

  if (nullptr == positionEvent)
    return;

  if (m_PlaneGeometry.IsNotNull())
  {
    // Check if the point is in the correct slice
    if (m_PlaneGeometry->DistanceFromPlane(positionEvent->GetPositionInWorld()) > mitk::sqrteps)
      return;
  }

  // Add repulsive points to avoid getting the same path again
  std::for_each(m_LiveWireContour->IteratorBegin(), m_LiveWireContour->IteratorEnd(), [this](ContourElement::VertexType *vertex) {
    ImageLiveWireContourModelFilter::InternalImageType::IndexType idx;
    this->m_ReferenceDataSlice->GetGeometry()->WorldToIndex(vertex->Coordinates, idx);
    this->m_LiveWireFilter->AddRepulsivePoint(idx);
    this->m_LiveWireFilterClosure->AddRepulsivePoint(idx);
  });

  // Remove duplicate first vertex, it's already contained in m_Contour
  m_LiveWireContour->RemoveVertexAt(0);

  // Set last point as control point
  m_LiveWireContour->SetControlVertexAt(m_LiveWireContour->GetNumberOfVertices() - 1);

  // Merge contours
  m_Contour->Concatenate(m_LiveWireContour);

  // Clear the LiveWire contour and reset the corresponding DataNode
  m_LiveWireContour->Clear();

  // Set new start point
  m_LiveWireFilter->SetStartPoint(positionEvent->GetPositionInWorld());
  m_LiveWireFilterClosure->SetStartPoint(positionEvent->GetPositionInWorld());

  if (m_CreateAndUseDynamicCosts)
  {
    // Use dynamic cost map for next update
    m_LiveWireFilter->CreateDynamicCostMap(m_Contour);
    m_LiveWireFilter->SetUseDynamicCostMap(true);

    m_LiveWireFilterClosure->CreateDynamicCostMap(m_Contour);
    m_LiveWireFilterClosure->SetUseDynamicCostMap(true);
  }

  mitk::RenderingManager::GetInstance()->RequestUpdate(positionEvent->GetSender()->GetRenderWindow());
}

void mitk::LiveWireTool2D::OnMouseMoved(StateMachineAction *, InteractionEvent *interactionEvent)
{
  // Compute LiveWire segment from last control point to current mouse position

  auto positionEvent = dynamic_cast<mitk::InteractionPositionEvent *>(interactionEvent);

  if (nullptr == positionEvent)
    return;

  if (m_PlaneGeometry.IsNotNull())
  {
    // Check if the point is in the correct slice
    if (m_PlaneGeometry->DistanceFromPlane(positionEvent->GetPositionInWorld()) > mitk::sqrteps)
      return;
  }

  //if (m_ClosureContour->IsEmpty())
  //{
  //    m_ClosureContour->AddVertex(m_Contour->GetVertexAt(0)->Coordinates);
  //    m_ClosureContour->AddVertex(positionEvent->GetPositionInWorld());
  //}
  //else
  //{
  //    m_ClosureContour->SetVertexAt(1, positionEvent->GetPositionInWorld());
  //}

  m_LiveWireFilter->SetEndPoint(positionEvent->GetPositionInWorld());
  m_LiveWireFilter->Update();

  m_LiveWireFilterClosure->SetStartPoint(positionEvent->GetPositionInWorld());
  m_LiveWireFilterClosure->Update();

  this->UpdateLiveWireContour();

  RenderingManager::GetInstance()->RequestUpdate(positionEvent->GetSender()->GetRenderWindow());
}

void mitk::LiveWireTool2D::OnMouseMoveNoDynamicCosts(StateMachineAction *, InteractionEvent *interactionEvent)
{
  m_LiveWireFilter->SetUseDynamicCostMap(false);
  m_LiveWireFilterClosure->SetUseDynamicCostMap(false);
  this->OnMouseMoved(nullptr, interactionEvent);
  m_LiveWireFilter->SetUseDynamicCostMap(true);
  m_LiveWireFilterClosure->SetUseDynamicCostMap(true);
}

bool mitk::LiveWireTool2D::OnCheckPoint(const InteractionEvent *interactionEvent)
{
  // Check double click on first control point to finish the LiveWire tool

  auto positionEvent = dynamic_cast<const mitk::InteractionPositionEvent *>(interactionEvent);

  if (nullptr == positionEvent)
    return false;

  mitk::Point3D click = positionEvent->GetPositionInWorld();
  mitk::Point3D first = this->m_Contour->GetVertexAt(0)->Coordinates;

  return first.EuclideanDistanceTo(click) < 4.5;
}

void mitk::LiveWireTool2D::OnFinish(StateMachineAction *, InteractionEvent *interactionEvent)
{
  // Finish LiveWire tool interaction

  m_Contour->Concatenate(m_ClosureContour);
  auto positionEvent = dynamic_cast<mitk::InteractionPositionEvent *>(interactionEvent);

  if (nullptr == positionEvent)
    return;

  if (m_PlaneGeometry.IsNotNull())
  {
    // Check if the point is in the correct slice
    if (m_PlaneGeometry->DistanceFromPlane(positionEvent->GetPositionInWorld()) > mitk::sqrteps)
      return;
  }

  //m_Contour->AddVertex(m_Contour->GetVertexAt(0)->Coordinates, false);
  // Remove last control point added by double click, if double click was performed on first point
  //if (OnCheckPoint(interactionEvent))
  //    m_Contour->RemoveVertexAt(m_Contour->GetNumberOfVertices() - 1);

  // remove green connection between mouse position and start point
  m_ClosureContour->Clear();

  // Save contour and corresponding plane geometry to list
  this->m_WorkingContours.emplace_back(std::make_pair(m_ContourNode, positionEvent->GetSender()->GetCurrentWorldPlaneGeometry()->Clone()));
  this->m_EditingContours.emplace_back(std::make_pair(m_EditingContourNode, positionEvent->GetSender()->GetCurrentWorldPlaneGeometry()->Clone()));

  m_LiveWireFilter->SetUseDynamicCostMap(false);
  m_LiveWireFilterClosure->SetUseDynamicCostMap(false);

  this->FinishTool();
}

void mitk::LiveWireTool2D::FinishTool()
{
  auto numberOfTimesteps = static_cast<int>(m_Contour->GetTimeSteps());

  for (int i = 0; i <= numberOfTimesteps; ++i)
    m_Contour->Close(i);

  this->GetToolManager()->GetDataStorage()->Remove(m_LiveWireContourNode);

  m_LiveWireContourNode = nullptr;
  m_LiveWireContour = nullptr;

  m_ContourInteractor = mitk::ContourModelLiveWireInteractor::New();
  m_ContourInteractor->SetDataNode(m_ContourNode);
  m_ContourInteractor->LoadStateMachine("ContourModelModificationInteractor.xml", us::GetModuleContext()->GetModule());
  m_ContourInteractor->SetEventConfig("ContourModelModificationConfig.xml", us::GetModuleContext()->GetModule());
  m_ContourInteractor->SetWorkingImage(this->m_ReferenceDataSlice);
  m_ContourInteractor->SetEditingContourModelNode(this->m_EditingContourNode);

  m_ContourNode->SetDataInteractor(m_ContourInteractor.GetPointer());

  this->m_LiveWireInteractors.push_back(m_ContourInteractor);
}

void mitk::LiveWireTool2D::OnLastSegmentDelete(StateMachineAction *, InteractionEvent *interactionEvent)
{
  // If last point of current contour will be removed go to start state and remove nodes
  if (m_Contour->GetNumberOfVertices() <= 1)
  {
    auto dataStorage = this->GetToolManager()->GetDataStorage();

    dataStorage->Remove(m_LiveWireContourNode);
    dataStorage->Remove(m_ContourNode);
    dataStorage->Remove(m_EditingContourNode);

    m_LiveWireContour = this->CreateNewContour();
    m_LiveWireContourNode->SetData(m_LiveWireContour);

    m_Contour = this->CreateNewContour();
    m_ContourNode->SetData(m_Contour);

    this->ResetToStartState();
  }
  else // Remove last segment from contour and reset LiveWire contour
  {
    m_LiveWireContour = this->CreateNewContour();
    m_LiveWireContourNode->SetData(m_LiveWireContour);

    auto newContour = this->CreateNewContour();

    auto begin = m_Contour->IteratorBegin();

    // Iterate from last point to next active point
    auto newLast = m_Contour->IteratorBegin() + (m_Contour->GetNumberOfVertices() - 1);

    // Go at least one down
    if (newLast != begin)
      --newLast;

    // Search next active control point
    while (newLast != begin && !((*newLast)->IsControlPoint))
      --newLast;

    // Set position of start point for LiveWire filter to coordinates of the new last point
    m_LiveWireFilter->SetStartPoint((*newLast)->Coordinates);
    //m_LiveWireFilterClosure->SetStartPoint((*newLast)->Coordinates);

    auto it = m_Contour->IteratorBegin();

    // Fll new Contour
    while (it <= newLast)
    {
      newContour->AddVertex((*it)->Coordinates, (*it)->IsControlPoint);
      ++it;
    }

    newContour->SetClosed(m_Contour->IsClosed());

    m_ContourNode->SetData(newContour);
    m_Contour = newContour;

    mitk::RenderingManager::GetInstance()->RequestUpdate(interactionEvent->GetSender()->GetRenderWindow());
  }
}

template <typename TPixel, unsigned int VImageDimension>
void mitk::LiveWireTool2D::FindHighestGradientMagnitudeByITK(itk::Image<TPixel, VImageDimension> *inputImage,
                                                             itk::Index<3> &index,
                                                             itk::Index<3> &returnIndex)
{
  typedef itk::Image<TPixel, VImageDimension> InputImageType;
  typedef typename InputImageType::IndexType IndexType;

  const auto MAX_X = inputImage->GetLargestPossibleRegion().GetSize()[0];
  const auto MAX_Y = inputImage->GetLargestPossibleRegion().GetSize()[1];

  returnIndex[0] = index[0];
  returnIndex[1] = index[1];
  returnIndex[2] = 0.0;

  double gradientMagnitude = 0.0;
  double maxGradientMagnitude = 0.0;

  // The size and thus the region of 7x7 is only used to calculate the gradient magnitude in that region,
  // not for searching the maximum value.

  // Maximum value in each direction for size
  typename InputImageType::SizeType size;
  size[0] = 7;
  size[1] = 7;

  // Minimum value in each direction for startRegion
  IndexType startRegion;
  startRegion[0] = index[0] - 3;
  startRegion[1] = index[1] - 3;
  if (startRegion[0] < 0)
    startRegion[0] = 0;
  if (startRegion[1] < 0)
    startRegion[1] = 0;
  if (MAX_X - index[0] < 7)
    startRegion[0] = MAX_X - 7;
  if (MAX_Y - index[1] < 7)
    startRegion[1] = MAX_Y - 7;

  index[0] = startRegion[0] + 3;
  index[1] = startRegion[1] + 3;

  typename InputImageType::RegionType region;
  region.SetSize(size);
  region.SetIndex(startRegion);

  typedef typename itk::GradientMagnitudeImageFilter<InputImageType, InputImageType> GradientMagnitudeFilterType;
  typename GradientMagnitudeFilterType::Pointer gradientFilter = GradientMagnitudeFilterType::New();
  gradientFilter->SetInput(inputImage);
  gradientFilter->GetOutput()->SetRequestedRegion(region);

  gradientFilter->Update();
  typename InputImageType::Pointer gradientMagnitudeImage;
  gradientMagnitudeImage = gradientFilter->GetOutput();

  IndexType currentIndex;
  currentIndex[0] = 0;
  currentIndex[1] = 0;

  // Search max (approximate) gradient magnitude
  for (int x = -1; x <= 1; ++x)
  {
    currentIndex[0] = index[0] + x;

    for (int y = -1; y <= 1; ++y)
    {
      currentIndex[1] = index[1] + y;
      gradientMagnitude = gradientMagnitudeImage->GetPixel(currentIndex);

      // Check for new max
      if (maxGradientMagnitude < gradientMagnitude)
      {
        maxGradientMagnitude = gradientMagnitude;
        returnIndex[0] = currentIndex[0];
        returnIndex[1] = currentIndex[1];
        returnIndex[2] = 0.0;
      }
    }

    currentIndex[1] = index[1];
  }
}
