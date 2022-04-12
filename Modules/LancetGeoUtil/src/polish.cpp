#include "polish.h"

//#include "mitkArithmeticOperation.h"
#include "mitkGeometryData.h"
#include "mitkImageReadAccessor.h"
#include "mitkSurfaceToImageFilter.h"
#include <vtkAppendPolyData.h>
#include <vtkBox.h>
#include <vtkClipPolyData.h>
#include <vtkDiscreteFlyingEdges3D.h>
#include <vtkFeatureEdges.h>
#include <vtkStripper.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkWindowedSincPolyDataFilter.h>


typedef itk::MutexLockHolder<itk::FastMutexLock> MutexLockHolder;

Timer::Timer()
{
  m_time_start = clock();
}

Timer::Timer(std::string name)
{
  m_time_start = clock();
  m_name = name;
}

Timer::~Timer()
{
  m_time_past = float(clock() - m_time_start) / CLOCKS_PER_SEC;
  MITK_INFO << m_name << ":" << m_time_past;
}

void Polish::Execute(Object *caller, const itk::EventObject &event)
{
  Execute((const itk::Object *)caller, event);
}

void Polish::Execute(const Object *caller, const itk::EventObject &event)
{
  PolishWorkflow();
}

Polish::Polish()
  : m_ThreadID(0),
    m_State(Setup),
    m_StopPolish(false)
{
  m_boneImage_polished = mitk::Image::New();
  m_boneImage = mitk::Image::New();
  m_boneSurface_polished = mitk::Surface::New();
  m_toolSurface = mitk::Surface::New();
  m_boneSurface = mitk::Surface::New();

  m_flyingEdgeFilter = vtkDiscreteFlyingEdges3D::New();
  m_wsFilter = vtkWindowedSincPolyDataFilter::New();
  m_surface2imagefilter = mitk::SurfaceToImageFilter::New();

  m_MultiThreader = itk::MultiThreader::New();
  m_PolishFinishedMutex = itk::FastMutexLock::New();
  m_StateMutex = itk::FastMutexLock::New();
  m_StopPolishMutex = itk::FastMutexLock::New();
}

Polish::~Polish()
{
  StopPolishing();
}

void Polish::SetState(PolishState state)
{
  itkDebugMacro("setting  m_State to " << state);

  MutexLockHolder lock(*m_StateMutex); // lock and unlock the mutex
  if (m_State == state)
  {
    return;
  }
  m_State = state;
  this->Modified();
}

Polish::PolishState Polish::GetState() const
{
  MutexLockHolder lock(*m_StateMutex);
  return m_State;
}

bool Polish::StartPolishing()
{
  if (m_boneImage.IsNotNull() && m_boneSurface.IsNotNull() && m_toolSurface.IsNotNull())
  {
    SetState(Ready);
  }
  //m_toolSurface->GetGeometry()->AddObserver(itk::ModifiedEvent(), this);
  if (this->GetState() != Ready)
    return false;

  this->SetState(Polishing);       // go to mode Tracking
  this->m_StopPolishMutex->Lock(); // update the local copy of m_StopTracking
  this->m_StopPolish = false;
  this->m_StopPolishMutex->Unlock();

  m_ThreadID = m_MultiThreader->SpawnThread(this->ThreadStartPolishing, this);
  // start a new thread that executes the PolishWorkflow() method
  //mitk::IGTTimeStamp::GetInstance()->Start(this);
  MITK_INFO << "Polish start!";
  return true;
}

bool Polish::StopPolishing()
{
  if (this->GetState() == Polishing) // Only if the object is in the correct state
  {
    m_StopPolishMutex->Lock(); // m_StopTracking is used by two threads, so we have to ensure correct thread handling
    m_StopPolish = true;
    m_StopPolishMutex->Unlock();
    // we have to wait here that the other thread recognizes the STOP-command and executes it
    m_PolishFinishedMutex->Lock();

    // StopPolishing was called, thus the mode should be changed back
    //   to Ready now that the tracking loop has ended.
    this->SetState(Ready);
    m_PolishFinishedMutex->Unlock();
  }
  return true;
}

void Polish::RestorePolish()
{
  StopPolishing();

  CopyVolume(m_boneImage_polished, m_boneImage);
  m_boneSurface_polished->SetVtkPolyData(m_boneSurface->GetVtkPolyData());
}

void Polish::SetboneSurface(mitk::Surface::Pointer boneSurface)
{
  if (m_boneSurface != boneSurface)
  {
    m_boneSurface = boneSurface;
    m_boneSurface_polished = boneSurface->Clone();
    this->Modified();
  }
}

void Polish::SetboneImage(mitk::Image::Pointer boneImage)
{
  if (m_boneImage != boneImage)
  {
    m_boneImage = boneImage;
    m_boneImage_polished = boneImage->Clone();
    this->Modified();
  }
}
void Polish::doPolish()
{
  MITK_INFO << "doPolish called";
  /* lock the TrackingFinishedMutex to signal that the execution rights are now transfered to the tracking thread */
  MutexLockHolder polishFinishedLockHolder(*m_PolishFinishedMutex); // keep lock until end of scope

  if (this->GetState() != Polishing)
  {
    MITK_INFO << "doPolish Return: state not Polishing";
    return;
  }

  bool localStopTracking;
  // Because m_StopTracking is used by two threads, access has to be guarded by a mutex. To minimize thread locking, a local copy is used here
  this->m_StopPolishMutex->Lock(); // update the local copy of m_StopTracking
  localStopTracking = this->m_StopPolish;
  this->m_StopPolishMutex->Unlock();
  while ((this->GetState() == Polishing) && (localStopTracking == false))
  {
    //MITK_INFO << "tracking";
    PolishWorkflow();
    /* Update the local copy of m_StopTracking */
    this->m_StopPolishMutex->Lock();
    localStopTracking = m_StopPolish;
    this->m_StopPolishMutex->Unlock();
  }
  /* StopPolishing was called, thus the mode should be changed back to Ready now that the polishing loop has ended. */

  //stop tracking
  MITK_INFO << " stop polishing";
  return;
  // returning from this function (and ThreadStartTracking()) this will end the thread and transfer control back to main thread by releasing trackingFinishedLockHolder
}

void Polish::PolishWorkflow()
{
  Timer timer{"polish"};
  //1.use tool surface to cut bone image
  SurfaceCutImage(m_toolSurface, m_boneImage_polished, true, true);
  //2.turn res image into surface
  DiscreteFlyingEdges3D(m_boneImage_polished);
}

void Polish::PolishWorkflow2()
{
  vtkNew<vtkAppendPolyData> appendPolydata;
  //1.gen implicit func from input tool surface
  vtkNew<vtkBox> vtkBox;
  vtkBox->SetBounds(m_toolSurface->GetVtkPolyData()->GetBounds());

  vtkBox->SetTransform(m_toolSurface->GetGeometry()->GetVtkTransform()->GetLinearInverse());
  //2.cut bone surface using implicit func
  vtkNew<vtkClipPolyData> cutter;
  cutter->SetClipFunction(vtkBox);
  cutter->SetInputData(m_boneSurface->GetVtkPolyData());
  cutter->Update();
  appendPolydata->AddInputData(cutter->GetOutput());
  //3.cap; Now extract feature edges
  vtkNew<vtkFeatureEdges> boundaryEdges;
  boundaryEdges->SetInputData(cutter->GetOutput());
  boundaryEdges->BoundaryEdgesOn();
  boundaryEdges->FeatureEdgesOff();
  boundaryEdges->NonManifoldEdgesOff();
  boundaryEdges->ManifoldEdgesOff();

  vtkNew<vtkStripper> boundaryStrips;
  boundaryStrips->SetInputConnection(boundaryEdges->GetOutputPort());
  boundaryStrips->Update();

  // Change the polylines into polygonsa
  vtkNew<vtkPolyData> boundaryPoly;
  boundaryPoly->SetPoints(boundaryStrips->GetOutput()->GetPoints());
  boundaryPoly->SetPolys(boundaryStrips->GetOutput()->GetLines());
  appendPolydata->AddInputData(boundaryPoly);
  appendPolydata->Update();
  mitk::Surface::Pointer res = mitk::Surface::New();
  res->SetVtkPolyData(appendPolydata->GetOutput());

  m_boneSurface->SetVtkPolyData(appendPolydata->GetOutput());
}

unsigned Polish::ThreadStartPolishing(void *pInfoStruct)
{
  MITK_INFO << "threadStartTracking Called";
  /* extract this pointer from Thread Info structure */
  struct itk::MultiThreader::ThreadInfoStruct *pInfo = (struct itk::MultiThreader::ThreadInfoStruct *)pInfoStruct;
  if (pInfo == nullptr)
  {
    return ITK_THREAD_RETURN_VALUE;
  }
  if (pInfo->UserData == nullptr)
  {
    return ITK_THREAD_RETURN_VALUE;
  }
  Polish *polish = (Polish *)pInfo->UserData;
  if (polish != nullptr)
  {
    polish->doPolish();
  }
  polish->m_ThreadID = 0; // erase thread id, now that this thread will end.
  return ITK_THREAD_RETURN_VALUE;
}

void Polish::CopyVolume(mitk::Image::Pointer dist, mitk::Image::Pointer src)
{
  try
  {
    mitk::ImageReadAccessor readAccess(src, src->GetVolumeData());
    dist->SetVolume(readAccess.GetData());
  }
  catch (mitk::Exception &e)
  {
    MITK_ERROR << e;
  }
}

mitk::Surface::Pointer Polish::DiscreteFlyingEdges3D(mitk::Image *mitkImage)
{
  Timer timer{"DiscreteFlyingEdges3D"};
  if (mitkImage == nullptr)
  {
    return nullptr;
  }
  //vtkSMPTools::Initialize(12);
  //auto matrix = mitkImage->GetGeometry()->GetVtkMatrix();
  vtkImageData *vtkImage = mitkImage->GetVtkImageData();

  double scale[3];
  mitkImage->GetGeometry()->GetSpacing().ToArray(scale);
  scale[0] = 1 / scale[0];
  scale[1] = 1 / scale[1];
  scale[2] = 1 / scale[2];

  vtkSmartPointer<vtkTransform> Transform = vtkSmartPointer<vtkTransform>::New();
  Transform->SetMatrix(mitkImage->GetGeometry()->GetVtkMatrix());
  Transform->Scale(scale);
  Transform->Update();

  // Create an isosurface

  static constexpr unsigned int smoothingIterations = 20;
  static constexpr double passBand = 0.01;
  static constexpr double featureAngle = 60.0;

  //const clock_t flyingEdges_time = clock();
  //flyingEdges3D->SetInputConnection(this->imageMathematics3->GetOutputPort());
  m_flyingEdgeFilter->SetInputData(vtkImage);
  m_flyingEdgeFilter->SetValue(0, 1);
  //m_flyingEdgeFilter->SetValue(1, 1);
  //flyingEdges3D->SetValue(1, 2);
  //flyingEdges3D->SetValue(2, 3);

  m_flyingEdgeFilter->SetComputeGradients(false);
  m_flyingEdgeFilter->SetComputeNormals(false);
  m_flyingEdgeFilter->SetComputeScalars(true);
  //m_flyingEdgeFilter->Update();
  // float secondflyingEdges = float(clock() - flyingEdges_time) / CLOCKS_PER_SEC;
  // MITK_INFO << "flyingEdges3D time is " << secondflyingEdges;
  //m_flyingEdgeFilter->Update();
  //const clock_t windowedSinc_time = clock();
  m_wsFilter->SetInputConnection(m_flyingEdgeFilter->GetOutputPort());
  m_wsFilter->SetNumberOfIterations(smoothingIterations);
  m_wsFilter->SetFeatureEdgeSmoothing(false);
  m_wsFilter->SetBoundarySmoothing(false);
  m_wsFilter->SetEdgeAngle(15);
  m_wsFilter->SetFeatureAngle(featureAngle);
  m_wsFilter->SetPassBand(passBand);
  //windowedSincPolyData->SetNonManifoldSmoothing(true);
  m_wsFilter->SetNonManifoldSmoothing(false); //no open
  m_wsFilter->SetNormalizeCoordinates(false);
  //m_wsFilter->Update();
  // float secondwindowedSinc = float(clock() - windowedSinc_time) / CLOCKS_PER_SEC;
  // MITK_INFO << "windowedSincPolyData time is " << secondwindowedSinc ;

  vtkSmartPointer<vtkTransformPolyDataFilter> TransformPolyDataFilter =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  TransformPolyDataFilter->SetTransform(Transform);
  TransformPolyDataFilter->SetInputConnection(m_wsFilter->GetOutputPort());
  //TransformPolyDataFilter->SetInputConnection(flyingEdges3D->GetOutputPort());
  TransformPolyDataFilter->Update();

  m_boneSurface_polished->SetVtkPolyData(TransformPolyDataFilter->GetOutput());
  //m_boneSurface->SetVtkPolyData(TransformPolyDataFilter->GetOutput());
  // mitk::Surface::Pointer res = mitk::Surface::New();
  // res->SetVtkPolyData(TransformPolyDataFilter->GetOutput());
  return m_boneSurface_polished;
  //surface->ShallowCopy(TransformPolyDataFilter->GetOutput());
}

mitk::Image::Pointer Polish::SurfaceCutImage(mitk::Surface *surface,
                                             mitk::Image *image,
                                             bool isReverseStencil,
                                             bool binary
  )
{
  Timer timer{"surfaceCutImage"};
  if (surface == nullptr || image == nullptr)
  {
    return nullptr;
  }
  // stencil

  m_surface2imagefilter->SetImage(image);
  m_surface2imagefilter->SetInput(surface);
  m_surface2imagefilter->SetReverseStencil(isReverseStencil);
  //m_surface2imagefilter->SetMakeOutputBinary(binary);
  m_surface2imagefilter->SetBackgroundValue(0);
  m_surface2imagefilter->Update();
  //Sleep(500);
  //mitk::Image::Pointer res = mitk::Image::New();
  m_boneImage_polished = m_surface2imagefilter->GetOutput()->Clone();
  //CopyVolume(m_boneImage, m_surface2imagefilter->GetOutput());
  return m_boneImage_polished;
}

