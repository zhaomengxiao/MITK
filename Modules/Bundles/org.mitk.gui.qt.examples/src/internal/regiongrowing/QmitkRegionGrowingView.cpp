/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Language:  C++
Date:      $Date$
Version:   $Revision: 17495 $ 
 
Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

// MITK
#include "mitkImageAccessByItk.h"
#include "mitkITKImageImport.h"
#include "mitkProperties.h"
#include "mitkColorProperty.h"
#include "mitkImageTimeSelector.h"

// Qmitk
#include "QmitkRegionGrowingView.h"
#include "QmitkPointListWidget.h"
#include "QmitkStdMultiWidget.h"
#include "mitkGlobalInteraction.h"

// Qt
#include <QMessageBox>

// ITK
#include <itkConnectedThresholdImageFilter.h>

const std::string QmitkRegionGrowingView::VIEW_ID = "org.mitk.views.regiongrowing";

QmitkRegionGrowingView::QmitkRegionGrowingView()
: QmitkFunctionality()
, m_Controls( 0 )
, m_MultiWidget( NULL )
{
}
QmitkRegionGrowingView::QmitkRegionGrowingView(const QmitkRegionGrowingView& other)
{
  Q_UNUSED(other)
  throw std::runtime_error("Copy constructor not implemented");
}

QmitkRegionGrowingView::~QmitkRegionGrowingView()
{
//    mitk::GlobalInteraction::GetInstance()->RemoveInteractor( m_PointSetInteractor );
//    m_PointSetInteractor = NULL;
//    m_PointSetNode->RemoveObserver(m_NodeDeleteObserverTag);
//    m_PointSetNode->RemoveObserver(m_NodeModifyObserverTag);
//    m_PointSet->RemoveObserver(m_PointSetAddObserverTag);
//    m_PointSet->RemoveObserver(m_PointSetMovedObserverTag);
}

void QmitkRegionGrowingView::Deactivated()
{
  //m_Controls->lstPoints->DeactivateInteractor(true);
  //TODO Deactivate Pointsetinteractor an remove observers
    m_PointSetInteractor = NULL;
    mitk::GlobalInteraction::GetInstance()->RemoveInteractor( m_PointSetInteractor );
}

void QmitkRegionGrowingView::CreateQtPartControl( QWidget *parent )
{
  // build up qt view, unless already done
  if ( !m_Controls )
  {
    // create GUI widgets from the Qt Designer's .ui file
    m_Controls = new Ui::QmitkRegionGrowingViewControls;
    m_Controls->setupUi( parent );
 
    //connect( m_Controls->btnPerformImageProcessing, SIGNAL(clicked()), this, SLOT(DoImageProcessing()) );
    //connect( m_Controls->btnSetSeedpoint, SIGNAL(toggled(bool)), this, SLOT(ActivatePointSetInteractor(bool)));
   
    // let the point set widget know about the multi widget (crosshair updates)
    //m_Controls->lstPoints->SetMultiWidget( m_MultiWidget );

    //connect(m_Controls->lstPoints, SIGNAL(PointSelectionChanged()), this, SLOT(OnSeedPointAdded()));

    //Replaced functionality of QmitkPointListWidget

    
    // create a new DataNode containing a PointSet with some interaction
    //m_PointSet = mitk::PointSet::New();

//    mitk::DataNode::Pointer pointSetNode = mitk::DataNode::New();
//    pointSetNode->SetData( m_PointSet );
//    pointSetNode->SetName("seed points for region growing");
//    pointSetNode->SetProperty("helper object", mitk::BoolProperty::New(true) );
//    pointSetNode->SetProperty("layer", mitk::IntProperty::New(1024) );
//    m_PointSetNode = mitk::DataNode::New();
//    m_PointSetNode->SetData( m_PointSet );
//    m_PointSetNode->SetName("seed points for region growing");
//    m_PointSetNode->SetProperty("helper object", mitk::BoolProperty::New(true) );
//    m_PointSetNode->SetProperty("layer", mitk::IntProperty::New(1024) );

//    itk::SimpleMemberCommand<QmitkRegionGrowingView>::Pointer deleteCommand = itk::SimpleMemberCommand<QmitkRegionGrowingView>::New();
//    deleteCommand->SetCallbackFunction( this, &QmitkRegionGrowingView::OnNodeDeleted );
//    m_NodeDeleteObserverTag = m_PointSetNode->AddObserver( itk::DeleteEvent(), deleteCommand );

//    itk::SimpleMemberCommand<QmitkRegionGrowingView>::Pointer modifyCommand = itk::SimpleMemberCommand<QmitkRegionGrowingView>::New();
//    modifyCommand->SetCallbackFunction( this, &QmitkRegionGrowingView::OnNodeModified );
//    m_NodeModifyObserverTag = m_PointSetNode->AddObserver( itk::ModifiedEvent(), modifyCommand );

//    itk::SimpleMemberCommand<QmitkRegionGrowingView>::Pointer pointAddedCommand = itk::SimpleMemberCommand<QmitkRegionGrowingView>::New();
//    pointAddedCommand->SetCallbackFunction(this, &QmitkRegionGrowingView::OnPointAdded);
//    m_PointSetAddObserverTag = m_PointSet->AddObserver( mitk::PointSetAddEvent(), pointAddedCommand);

//    itk::SimpleMemberCommand<QmitkRegionGrowingView>::Pointer pointMovedCommand = itk::SimpleMemberCommand<QmitkRegionGrowingView>::New();
//    pointMovedCommand->SetCallbackFunction(this, &QmitkRegionGrowingView::OnPointMoved);
//    m_PointSetMovedObserverTag = m_PointSet->AddObserver( mitk::PointSetMoveEvent(), pointMovedCommand);

    // add the pointset to the data tree (for rendering and access by other modules)
    //GetDefaultDataStorage()->Add( m_PointSetNode );

    // tell the GUI widget about out point set
    //m_Controls->lstPoints->SetPointSetNode( pointSetNode );

    m_Controls->m_AdaptiveRGWidget->SetDataStorage(this->GetDataStorage());
    m_Controls->m_AdaptiveRGWidget->CreateConnections();

//    m_Controls->m_AdaptiveRGWidget->setEnabled(false);
  }
}

void QmitkRegionGrowingView::NodeRemoved(const mitk::DataNode *node)
{
     MITK_INFO<<"Deleted....";
//     m_Controls->btnSetSeedpoint->setEnabled(false);

}

//void QmitkRegionGrowingView::OnPointAdded()
//{
//    if (m_PointSet->GetSize() == 2)
//    {
//        mitk::PointOperation* swapOp = new mitk::PointOperation(mitk::OpMOVEPOINTUP, m_PointSet->GetPoint(1), 1);
//        m_PointSet->ExecuteOperation(swapOp);
//        mitk::PointOperation* removeOp = new mitk::PointOperation(mitk::OpREMOVE, m_PointSet->GetPoint(1), 1);
//        m_PointSet->ExecuteOperation(removeOp);
//    }
//    this->OnSeedPointAdded();
//}

void QmitkRegionGrowingView::OnPointMoved()
{
    this->OnSeedPointAdded();
}

void QmitkRegionGrowingView::StdMultiWidgetAvailable (QmitkStdMultiWidget &stdMultiWidget)
{
  m_MultiWidget = &stdMultiWidget;
  m_Controls->m_AdaptiveRGWidget->SetMultiWidget(&stdMultiWidget);
}


void QmitkRegionGrowingView::StdMultiWidgetNotAvailable()
{
  m_MultiWidget = NULL;
}


void QmitkRegionGrowingView::OnSelectionChanged( std::vector<mitk::DataNode*> nodes )
{ 
  // iterate all selected objects, adjust warning visibility
  for( std::vector<mitk::DataNode*>::iterator it = nodes.begin();
       it != nodes.end();
       ++it )
  {
    mitk::DataNode::Pointer node = *it;
  
    if( node.IsNotNull() && dynamic_cast<mitk::Image*>(node->GetData()) )
    {
      m_Controls->lblWarning->setVisible( false );
      m_Controls->m_AdaptiveRGWidget->SetInputImageNode(node);
//      m_Controls->btnSetSeedpoint->setEnabled(true);
      return;
    }
  }

  m_Controls->lblWarning->setVisible( true );
//  m_Controls->btnSetSeedpoint->setEnabled(false);
}

void QmitkRegionGrowingView::ActivatePointSetInteractor(bool status)
{
    if (m_PointSetNode)
    {
        if(status)
        {
            m_PointSetInteractor = dynamic_cast<mitk::PointSetInteractor*>(m_PointSetNode->GetInteractor());

            if (m_PointSetInteractor.IsNull())//if not present, instanciate one
              m_PointSetInteractor = mitk::PointSetInteractor::New("pointsetinteractor", m_PointSetNode);

            mitk::GlobalInteraction::GetInstance()->AddInteractor( m_PointSetInteractor );
        }
        else
        {
            mitk::GlobalInteraction::GetInstance()->RemoveInteractor( m_PointSetInteractor );
            m_PointSetInteractor = NULL;
        }
    }
}

void QmitkRegionGrowingView::OnSeedPointAdded()
{
    mitk::Point3D seedPoint = m_PointSet->GetPointSet(0)->GetPoints()->ElementAt(0);

    std::vector<mitk::DataNode*> nodes = this->GetDataManagerSelection();
    if (nodes.empty()) return;

    mitk::DataNode* node = nodes.front();

    if (!node)
    {
      // Nothing selected. Inform the user and return
      QMessageBox::information( NULL, "Region growing functionality", "Please load and select an image before region growing.");
      return;
    }

    // here we have a valid mitk::DataNode

    // a node itself is not very useful, we need its data item (the image)
    mitk::BaseData* data = node->GetData();
    if (data)
    {
      // test if this data item is an image or not (could also be a surface or something totally different)
      mitk::Image* image = dynamic_cast<mitk::Image*>( data );
      if (image)
      {
          mitk::Index3D seedPointInIndex;
          mitk::Index3D currentIndex;
          mitk::ScalarType pixelValues[125];
          unsigned int pos (0);
          image->GetGeometry()->WorldToIndex(seedPoint, seedPointInIndex);

          for(int i = seedPointInIndex[0]-2; i <= seedPointInIndex[0]+2; i++)
          {
              for(int j = seedPointInIndex[1]-2; j <= seedPointInIndex[1]+2; j++)
              {
                  for(int k = seedPointInIndex[2]-2; k <= seedPointInIndex[2]+2; k++)
                  {
                      currentIndex[0] = i;
                      currentIndex[1] = j;
                      currentIndex[2] = k;
                      if(image->GetGeometry()->IsIndexInside(currentIndex))
                      {
                          pixelValues[pos] = image->GetPixelValueByIndex(currentIndex);
                          pos++;
                      }
                      else
                      {
                          pixelValues[pos] = -10000000;
                          pos++;
                      }
                  }
              }
          }

          //Now calculation mean and deviation of the pixelValues
          mitk::ScalarType mean(0);
          unsigned int numberOfValues(0);
          for (unsigned int i = 0; i < 125; i++)
          {
              if(pixelValues[i] > -10000000)
              {
                  mean += pixelValues[i];
                  numberOfValues++;
              }
          }

          mean = mean/numberOfValues;
          MITK_INFO<<"Mean: "<<mean;

          m_InitialThreshold = mean;

          mitk::ScalarType deviation(0);

          for (unsigned int i = 0; i < 125; i++)
          {
              if(pixelValues[i] > -10000000)
              {
                  deviation += pow((mean - pixelValues[i]),2);
              }
          }
          deviation = deviation/(numberOfValues-1);
          deviation = sqrt(deviation);

          MITK_INFO<<"SliderRange: "<<deviation*3;
          MITK_INFO<<"SliderValue: "<<deviation*1.5;

//          m_Controls->sliderOffsetValue->setRange(0,deviation*3);
//          m_Controls->sliderOffsetValue->setValue(1.5*deviation);

          MITK_INFO<<"Deviation: "<<deviation;
      }
    }
}

void QmitkRegionGrowingView::DoImageProcessing()
{
  std::vector<mitk::DataNode*> nodes = this->GetDataManagerSelection();
  if (nodes.empty()) return;

  mitk::DataNode* node = nodes.front();

  if (!node)
  {
    // Nothing selected. Inform the user and return
    QMessageBox::information( NULL, "Region growing functionality", "Please load and select an image before region growing.");
    return;
  }

  // here we have a valid mitk::DataNode

  // a node itself is not very useful, we need its data item (the image)
  mitk::BaseData* data = node->GetData();
  if (data)
  {
    // test if this data item is an image or not (could also be a surface or something totally different)
    mitk::Image* image = dynamic_cast<mitk::Image*>( data );
    if (image)
    {
      std::stringstream message;
      std::string name;
      message << "Performing image processing for image ";
      if (node->GetName(name))
      {
        // a property called "name" was found for this DataNode
        message << "'" << name << "'";
      }
      message << ".";
      MITK_INFO << message.str();

      // So we have an image. Get the current time step to see if the user has set some seed points already
      unsigned int t = m_MultiWidget->GetTimeNavigationController()->GetTime()->GetPos();

      if ( m_PointSet->GetSize(t) == 0 )
      {
        // no points there. Not good for region growing
        QMessageBox::information( NULL, "Region growing functionality", 
                                        "Please set some seed points inside the image first.\n"
                                        "(hold Shift key and click left mouse button inside the image.)"
                                );
        return;
      }

      // actually perform region growing. Here we have both an image and some seed points

      if (image->GetDimension() == 4)
      {
        mitk::ImageTimeSelector::Pointer timeSelector = mitk::ImageTimeSelector::New();
        timeSelector->SetInput(image);
        timeSelector->SetTimeNr(t);
        timeSelector->UpdateLargestPossibleRegion();
        mitk::Image::Pointer image3D = timeSelector->GetOutput();
//        AccessByItk_n(image3D, ItkImageProcessing, (image3D->GetGeometry(), node, m_Controls->sliderOffsetValue->value(), t));
      }
      else
      {
//        AccessByItk_n(image, ItkImageProcessing, (image->GetGeometry(), node, m_Controls->sliderOffsetValue->value(), t)); // some magic to call the correctly templated function
      }
    }
  }
}


template < typename TPixel, unsigned int VImageDimension >
void QmitkRegionGrowingView::ItkImageProcessing( itk::Image< TPixel, VImageDimension >* itkImage, mitk::Geometry3D* imageGeometry, mitk::DataNode* parent, int thresholdOffset, unsigned int t)
{
  typedef itk::Image< TPixel, VImageDimension >        InputImageType;
  typedef typename InputImageType::IndexType           IndexType;
  typedef itk::Image< unsigned char, VImageDimension > OutputImageType;
  
  // instantiate an ITK region growing filter, set its parameters
  typedef itk::ConnectedThresholdImageFilter<InputImageType, OutputImageType> RegionGrowingFilterType;
  typename RegionGrowingFilterType::Pointer regionGrower = RegionGrowingFilterType::New();
  regionGrower->SetInput( itkImage ); // don't forget this

  // determine a thresholding interval
  IndexType seedIndex;
  TPixel min( std::numeric_limits<TPixel>::max() );
  TPixel max( std::numeric_limits<TPixel>::min() );
  mitk::PointSet::PointsContainer* points = m_PointSet->GetPointSet(t)->GetPoints();
  for ( mitk::PointSet::PointsConstIterator pointsIterator = points->Begin(); 
        pointsIterator != points->End();
        ++pointsIterator ) 
  {
    // first test if this point is inside the image at all
    if ( !imageGeometry->IsInside( pointsIterator.Value()) ) 
    {
      continue;
    }

    // convert world coordinates to image indices
    imageGeometry->WorldToIndex( pointsIterator.Value(), seedIndex);

    // get the pixel value at this point
    TPixel currentPixelValue = itkImage->GetPixel( seedIndex );

    // adjust minimum and maximum values
    if (currentPixelValue > max)
      max = currentPixelValue;

    if (currentPixelValue < min)
      min = currentPixelValue;

    regionGrower->AddSeed( seedIndex );
  }

//  std::cout << "Values between " << min << " and " << max << std::endl;

//  std::cout << "Values between " << m_InitialThreshold-m_Controls->sliderOffsetValue->value() << " and " << m_InitialThreshold+m_Controls->sliderOffsetValue->value() << std::endl;

  min -= thresholdOffset;
  max += thresholdOffset;

  // set thresholds and execute filter
//  regionGrower->SetLower( min );
//  regionGrower->SetUpper( max );

//  regionGrower->SetLower( m_InitialThreshold-m_Controls->sliderOffsetValue->value() );
//  regionGrower->SetUpper( m_InitialThreshold+m_Controls->sliderOffsetValue->value() );

  regionGrower->Update();

  mitk::Image::Pointer resultImage = mitk::ImportItkImage( regionGrower->GetOutput() );
  mitk::DataNode::Pointer newNode = mitk::DataNode::New();
  newNode->SetData( resultImage );

  // set some properties
  newNode->SetProperty("binary", mitk::BoolProperty::New(true));
  newNode->SetProperty("name", mitk::StringProperty::New("dumb segmentation"));
  newNode->SetProperty("color", mitk::ColorProperty::New(1.0,0.0,0.0));
  newNode->SetProperty("volumerendering", mitk::BoolProperty::New(false));
  newNode->SetProperty("layer", mitk::IntProperty::New(1));
  newNode->SetProperty("opacity", mitk::FloatProperty::New(0.5));

  // add result to data tree
  this->GetDefaultDataStorage()->Add( newNode, parent );
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}
