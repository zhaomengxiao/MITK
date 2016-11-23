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

#include "mitkTool.h"

#include "mitkAnatomicalStructureColorPresets.h"
#include "mitkDICOMProperty.h"
#include "mitkDisplayInteractor.h"
#include "mitkIDICOMTagsOfInterest.h"
#include "mitkImageReadAccessor.h"
#include "mitkImageWriteAccessor.h"
#include "mitkLabelSetImage.h"
#include "mitkLevelWindowProperty.h"
#include "mitkLookupTableProperty.h"
#include "mitkProperties.h"
#include "mitkPropertyNameHelper.h"
#include "mitkVtkResliceInterpolationProperty.h"

// us
#include <usGetModuleContext.h>
#include <usModuleResource.h>

// itk
#include <itkObjectFactory.h>

mitk::Tool::Tool(const char *type)
  : m_PredicateImages(NodePredicateDataType::New("Image")) // for reference images
    ,
    m_PredicateDim3(NodePredicateDimension::New(3, 1)),
    m_PredicateDim4(NodePredicateDimension::New(4, 1)),
    m_PredicateDimension(mitk::NodePredicateOr::New(m_PredicateDim3, m_PredicateDim4)),
    m_PredicateImage3D(NodePredicateAnd::New(m_PredicateImages, m_PredicateDimension)),
    m_PredicateBinary(NodePredicateProperty::New("binary", BoolProperty::New(true))),
    m_PredicateNotBinary(NodePredicateNot::New(m_PredicateBinary)),
    m_PredicateSegmentation(NodePredicateProperty::New("segmentation", BoolProperty::New(true))),
    m_PredicateNotSegmentation(NodePredicateNot::New(m_PredicateSegmentation)),
    m_PredicateHelper(NodePredicateProperty::New("helper object", BoolProperty::New(true))),
    m_PredicateNotHelper(NodePredicateNot::New(m_PredicateHelper)),
    m_PredicateImageColorful(NodePredicateAnd::New(m_PredicateNotBinary, m_PredicateNotSegmentation)),
    m_PredicateImageColorfulNotHelper(NodePredicateAnd::New(m_PredicateImageColorful, m_PredicateNotHelper)),
    m_PredicateReference(NodePredicateAnd::New(m_PredicateImage3D, m_PredicateImageColorfulNotHelper)),
    m_IsSegmentationPredicate(
      NodePredicateAnd::New(NodePredicateOr::New(m_PredicateBinary, m_PredicateSegmentation), m_PredicateNotHelper)),
    m_InteractorType(type),
    m_DisplayInteractorConfigs(),
    m_EventConfig("DisplayConfigMITK.xml")
{
}

mitk::Tool::~Tool()
{
}

bool mitk::Tool::CanHandle(BaseData *) const
{
  return true;
}

void mitk::Tool::InitializeStateMachine()
{
  if (m_InteractorType.empty())
    return;

  m_InteractorType += ".xml";

  try
  {
    LoadStateMachine(m_InteractorType, us::GetModuleContext()->GetModule());
    SetEventConfig("SegmentationToolsConfig.xml", us::GetModuleContext()->GetModule());
  }
  catch (const std::exception &e)
  {
    MITK_ERROR << "Could not load statemachine pattern " << m_InteractorType << " with exception: " << e.what();
  }
}

void mitk::Tool::Notify(InteractionEvent *interactionEvent, bool isHandled)
{
  // to use the state machine pattern,
  // the event is passed to the state machine interface to be handled
  if (!isHandled)
  {
    this->HandleEvent(interactionEvent, nullptr);
  }
}

void mitk::Tool::ConnectActionsAndFunctions()
{
}

bool mitk::Tool::FilterEvents(InteractionEvent *, DataNode *)
{
  return true;
}

const char *mitk::Tool::GetGroup() const
{
  return "default";
}

void mitk::Tool::SetToolManager(ToolManager *manager)
{
  m_ToolManager = manager;
}

void mitk::Tool::Activated()
{
  // As a legacy solution the display interaction of the new interaction framework is disabled here to avoid conflicts
  // with tools
  // Note: this only affects InteractionEventObservers (formerly known as Listeners) all DataNode specific interaction
  // will still be enabled
  m_DisplayInteractorConfigs.clear();
  std::vector<us::ServiceReference<InteractionEventObserver>> listEventObserver =
    us::GetModuleContext()->GetServiceReferences<InteractionEventObserver>();
  for (std::vector<us::ServiceReference<InteractionEventObserver>>::iterator it = listEventObserver.begin();
       it != listEventObserver.end();
       ++it)
  {
    DisplayInteractor *displayInteractor =
      dynamic_cast<DisplayInteractor *>(us::GetModuleContext()->GetService<InteractionEventObserver>(*it));
    if (displayInteractor != nullptr)
    {
      // remember the original configuration
      m_DisplayInteractorConfigs.insert(std::make_pair(*it, displayInteractor->GetEventConfig()));
      // here the alternative configuration is loaded
      displayInteractor->SetEventConfig(m_EventConfig.c_str());
    }
  }
}

void mitk::Tool::Deactivated()
{
  // Re-enabling InteractionEventObservers that have been previously disabled for legacy handling of Tools
  // in new interaction framework
  for (std::map<us::ServiceReferenceU, EventConfig>::iterator it = m_DisplayInteractorConfigs.begin();
       it != m_DisplayInteractorConfigs.end();
       ++it)
  {
    if (it->first)
    {
      DisplayInteractor *displayInteractor =
        static_cast<DisplayInteractor *>(us::GetModuleContext()->GetService<InteractionEventObserver>(it->first));
      if (displayInteractor != nullptr)
      {
        // here the regular configuration is loaded again
        displayInteractor->SetEventConfig(it->second);
      }
    }
  }
  m_DisplayInteractorConfigs.clear();
}

itk::Object::Pointer mitk::Tool::GetGUI(const std::string &toolkitPrefix, const std::string &toolkitPostfix)
{
  itk::Object::Pointer object;

  std::string classname = this->GetNameOfClass();
  std::string guiClassname = toolkitPrefix + classname + toolkitPostfix;

  std::list<itk::LightObject::Pointer> allGUIs = itk::ObjectFactoryBase::CreateAllInstance(guiClassname.c_str());
  for (std::list<itk::LightObject::Pointer>::iterator iter = allGUIs.begin(); iter != allGUIs.end(); ++iter)
  {
    if (object.IsNull())
    {
      object = dynamic_cast<itk::Object *>(iter->GetPointer());
    }
    else
    {
      MITK_ERROR << "There is more than one GUI for " << classname << " (several factories claim ability to produce a "
                 << guiClassname << " ) " << std::endl;
      return nullptr; // people should see and fix this error
    }
  }

  return object;
}

mitk::NodePredicateBase::ConstPointer mitk::Tool::GetReferenceDataPreference() const
{
  return m_PredicateReference.GetPointer();
}

mitk::NodePredicateBase::ConstPointer mitk::Tool::GetWorkingDataPreference() const
{
  return m_IsSegmentationPredicate.GetPointer();
}

mitk::DataNode::Pointer mitk::Tool::CreateEmptySegmentationNode(Image *original,
                                                                const std::string &organName,
                                                                const mitk::Color &color)
{
  // we NEED a reference image for size etc.
  if (!original)
    return nullptr;

  // actually create a new empty segmentation
  PixelType pixelType(mitk::MakeScalarPixelType<DefaultSegmentationDataType>());
  LabelSetImage::Pointer segmentation = LabelSetImage::New();

  if (original->GetDimension() == 2)
  {
    const unsigned int dimensions[] = {original->GetDimension(0), original->GetDimension(1), 1};
    segmentation->Initialize(pixelType, 3, dimensions);
    segmentation->AddLayer();
  }
  else
  {
    segmentation->Initialize(original);
  }

  mitk::Label::Pointer label = mitk::Label::New();
  label->SetName(organName);
  label->SetColor(color);
  label->SetValue(1);
  segmentation->GetActiveLabelSet()->AddLabel(label);
  segmentation->GetActiveLabelSet()->SetActiveLabel(1);

  unsigned int byteSize = sizeof(mitk::Label::PixelType);

  if (segmentation->GetDimension() < 4)
  {
    for (unsigned int dim = 0; dim < segmentation->GetDimension(); ++dim)
    {
      byteSize *= segmentation->GetDimension(dim);
    }

    mitk::ImageWriteAccessor writeAccess(segmentation.GetPointer(), segmentation->GetVolumeData(0));

    memset(writeAccess.GetData(), 0, byteSize);
  }
  else
  {
    // if we have a time-resolved image we need to set memory to 0 for each time step
    for (unsigned int dim = 0; dim < 3; ++dim)
    {
      byteSize *= segmentation->GetDimension(dim);
    }

    for (unsigned int volumeNumber = 0; volumeNumber < segmentation->GetDimension(3); volumeNumber++)
    {
      mitk::ImageWriteAccessor writeAccess(segmentation.GetPointer(), segmentation->GetVolumeData(volumeNumber));

      memset(writeAccess.GetData(), 0, byteSize);
    }
  }

  if (original->GetTimeGeometry())
  {
    TimeGeometry::Pointer originalGeometry = original->GetTimeGeometry()->Clone();
    segmentation->SetTimeGeometry(originalGeometry);
  }
  else
  {
    Tool::ErrorMessage("Original image does not have a 'Time sliced geometry'! Cannot create a segmentation.");
    return nullptr;
  }

  // Add some DICOM Tags as properties to segmentation image
  AddDICOMTagsToSegmentation(original, segmentation, organName, color);

  return CreateSegmentationNode(segmentation, organName, color);
}

mitk::DataNode::Pointer mitk::Tool::CreateSegmentationNode(Image *image,
                                                           const std::string &organName,
                                                           const mitk::Color &color)
{
  if (!image)
    return nullptr;

  // decorate the datatreenode with some properties
  DataNode::Pointer segmentationNode = DataNode::New();
  segmentationNode->SetData(image);

  // name
  segmentationNode->SetProperty("name", StringProperty::New(organName));

  // visualization properties
  segmentationNode->SetProperty("binary", BoolProperty::New(true));
  segmentationNode->SetProperty("color", ColorProperty::New(color));
  mitk::LookupTable::Pointer lut = mitk::LookupTable::New();
  lut->SetType(mitk::LookupTable::MULTILABEL);
  mitk::LookupTableProperty::Pointer lutProp = mitk::LookupTableProperty::New();
  lutProp->SetLookupTable(lut);
  segmentationNode->SetProperty("LookupTable", lutProp);
  segmentationNode->SetProperty("texture interpolation", BoolProperty::New(false));
  segmentationNode->SetProperty("layer", IntProperty::New(10));
  segmentationNode->SetProperty("levelwindow", LevelWindowProperty::New(LevelWindow(0.5, 1)));
  segmentationNode->SetProperty("opacity", FloatProperty::New(0.3));
  segmentationNode->SetProperty("segmentation", BoolProperty::New(true));
  segmentationNode->SetProperty("reslice interpolation",
                                VtkResliceInterpolationProperty::New()); // otherwise -> segmentation appears in 2
                                                                         // slices sometimes (only visual effect, not
                                                                         // different data)
  // For MITK-3M3 release, the volume of all segmentations should be shown
  segmentationNode->SetProperty("showVolume", BoolProperty::New(true));

  return segmentationNode;
}

void mitk::Tool::AddDICOMTagsToSegmentation(Image *original,
                                            Image *segmentation,
                                            const std::string &organName,
                                            const mitk::Color &color)
{
  mitk::AnatomicalStructureColorPresets::Category category;
  mitk::AnatomicalStructureColorPresets::Type type;
  mitk::AnatomicalStructureColorPresets *anatomicalStructureColorPresets = mitk::AnatomicalStructureColorPresets::New();
  anatomicalStructureColorPresets->LoadPreset();

  for (const auto &preset : anatomicalStructureColorPresets->GetCategoryPresets())
  {
    auto presetOrganName = preset.first;
    if (organName.compare(presetOrganName) == 0)
    {
      category = preset.second;
      break;
    }
  }

  for (const auto &preset : anatomicalStructureColorPresets->GetTypePresets())
  {
    auto presetOrganName = preset.first;
    if (organName.compare(presetOrganName) == 0)
    {
      type = preset.second;
      break;
    }
  }

  // Add DICOM Tag (0008, 0060) Modality "SEG"
  segmentation->SetProperty(mitk::GeneratePropertyNameForDICOMTag(0x0008, 0x0060).c_str(), StringProperty::New("SEG"));
  // Add DICOM Tag (0008,103E) Series Description
  segmentation->SetProperty(mitk::GeneratePropertyNameForDICOMTag(0x0008, 0x103E).c_str(),
                            StringProperty::New("MITK Segmentation"));
  //------------------------------------------------------------
  // Add Segment Sequence tags (0062, 0002)
  mitk::DICOMTagPath segmentSequencePath;
  segmentSequencePath.AddElement(0x0062, 0x0002);

  // Segment Number:Identification number of the segment.The value of Segment Number(0062, 0004) shall be unique within
  // the Segmentation instance in which it is created
  mitk::DICOMTagPath segmentNumberPath;
  segmentNumberPath.AddElement(0x0062, 0x0002).AddElement(0x0062, 0x0004);
  segmentation->SetProperty(mitk::DICOMTagPathToPropertyName(segmentNumberPath).c_str(), StringProperty::New("1"));

  // Segment Label: User-defined label identifying this segment.
  mitk::DICOMTagPath segmentLabelPath;
  segmentLabelPath.AddElement(0x0062, 0x0002).AddElement(0x0062, 0x0005);
  segmentation->SetProperty(mitk::DICOMTagPathToPropertyName(segmentLabelPath).c_str(), StringProperty::New(organName));

  // Segment Algorithm Type: Type of algorithm used to generate the segment. AUTOMATIC SEMIAUTOMATIC MANUAL
  mitk::DICOMTagPath segmentAlgorithmTypePath;
  segmentAlgorithmTypePath.AddElement(0x0062, 0x0002).AddElement(0x0062, 0x0008);
  segmentation->SetProperty(mitk::DICOMTagPathToPropertyName(segmentAlgorithmTypePath).c_str(),
                            StringProperty::New("SEMIAUTOMATIC"));
  //------------------------------------------------------------
  // Add Segmented Property Category Code Sequence tags (0062, 0003): Sequence defining the general category of this
  // segment.
  mitk::DICOMTagPath segmentSegmentedPropertyCategorySequencePath;
  segmentSegmentedPropertyCategorySequencePath.AddElement(0x0062, 0x0002).AddElement(0x0062, 0x0003);
  // (0008,0100) Code Value
  mitk::DICOMTagPath segmentCategoryCodeValuePath;
  segmentCategoryCodeValuePath.AddElement(0x0062, 0x0002).AddElement(0x0062, 0x0003).AddElement(0x008, 0x0100);
  if (!category.codeValue.empty())
    segmentation->SetProperty(mitk::DICOMTagPathToPropertyName(segmentCategoryCodeValuePath).c_str(),
                              StringProperty::New(category.codeValue));

  // (0008,0102) Coding Scheme Designator
  mitk::DICOMTagPath segmentCategoryCodeSchemePath;
  segmentCategoryCodeSchemePath.AddElement(0x0062, 0x0002).AddElement(0x0062, 0x0003).AddElement(0x008, 0x0102);
  if (!category.codeScheme.empty())
    segmentation->SetProperty(mitk::DICOMTagPathToPropertyName(segmentCategoryCodeSchemePath).c_str(),
                              StringProperty::New(category.codeScheme));

  // (0008,0104) Code Meaning
  mitk::DICOMTagPath segmentCategoryCodeMeaningPath;
  segmentCategoryCodeMeaningPath.AddElement(0x0062, 0x0002).AddElement(0x0062, 0x0003).AddElement(0x008, 0x0104);
  if (!category.codeName.empty())
    segmentation->SetProperty(mitk::DICOMTagPathToPropertyName(segmentCategoryCodeMeaningPath).c_str(),
                              StringProperty::New(category.codeName));
  //------------------------------------------------------------
  // Add Segmented Property Type Code Sequence (0062, 000F): Sequence defining the specific property type of this
  // segment.
  mitk::DICOMTagPath segmentSegmentedPropertyTypeSequencePath;
  segmentSegmentedPropertyTypeSequencePath.AddElement(0x0062, 0x0002).AddElement(0x0062, 0x000F);

  // (0008,0100) Code Value
  mitk::DICOMTagPath segmentTypeCodeValuePath;
  segmentTypeCodeValuePath.AddElement(0x0062, 0x0002).AddElement(0x0062, 0x000F).AddElement(0x008, 0x0100);
  if (!type.codeValue.empty())
    segmentation->SetProperty(mitk::DICOMTagPathToPropertyName(segmentTypeCodeValuePath).c_str(),
                              StringProperty::New(type.codeValue));

  // (0008,0102) Coding Scheme Designator
  mitk::DICOMTagPath segmentTypeCodeSchemePath;
  segmentTypeCodeSchemePath.AddElement(0x0062, 0x0002).AddElement(0x0062, 0x000F).AddElement(0x008, 0x0102);
  if (!type.codeScheme.empty())
    segmentation->SetProperty(mitk::DICOMTagPathToPropertyName(segmentTypeCodeSchemePath).c_str(),
                              StringProperty::New(type.codeScheme));

  // (0008,0104) Code Meaning
  mitk::DICOMTagPath segmentTypeCodeMeaningPath;
  segmentTypeCodeMeaningPath.AddElement(0x0062, 0x0002).AddElement(0x0062, 0x000F).AddElement(0x008, 0x0104);
  if (!type.codeName.empty())
    segmentation->SetProperty(mitk::DICOMTagPathToPropertyName(segmentTypeCodeMeaningPath).c_str(),
                              StringProperty::New(type.codeName));
  //------------------------------------------------------------
  // Add Segmented Property Type Modifier Code Sequence (0062,0011): Sequence defining the modifier of the property type
  // of this segment.
  mitk::DICOMTagPath segmentSegmentedPropertyModifierSequencePath;
  segmentSegmentedPropertyModifierSequencePath.AddElement(0x0062, 0x0002)
    .AddElement(0x0062, 0x000F)
    .AddElement(0x0062, 0x0011);
  // (0008,0100) Code Value
  mitk::DICOMTagPath segmentModifierCodeValuePath;
  segmentModifierCodeValuePath.AddElement(0x0062, 0x0002)
    .AddElement(0x0062, 0x000F)
    .AddElement(0x0062, 0x0011)
    .AddElement(0x008, 0x0100);
  if (!type.modifier.codeValue.empty())
    segmentation->SetProperty(mitk::DICOMTagPathToPropertyName(segmentModifierCodeValuePath).c_str(),
                              StringProperty::New(type.modifier.codeValue));

  // (0008,0102) Coding Scheme Designator
  mitk::DICOMTagPath segmentModifierCodeSchemePath;
  segmentModifierCodeSchemePath.AddElement(0x0062, 0x0002)
    .AddElement(0x0062, 0x000F)
    .AddElement(0x0062, 0x0011)
    .AddElement(0x008, 0x0102);
  if (!type.modifier.codeScheme.empty())
    segmentation->SetProperty(mitk::DICOMTagPathToPropertyName(segmentModifierCodeSchemePath).c_str(),
                              StringProperty::New(type.modifier.codeScheme));

  // (0008,0104) Code Meaning
  mitk::DICOMTagPath segmentModifierCodeMeaningPath;
  segmentModifierCodeMeaningPath.AddElement(0x0062, 0x0002)
    .AddElement(0x0062, 0x000F)
    .AddElement(0x0062, 0x0011)
    .AddElement(0x008, 0x0104);
  if (!type.modifier.codeName.empty())
    segmentation->SetProperty(mitk::DICOMTagPathToPropertyName(segmentModifierCodeMeaningPath).c_str(),
                              StringProperty::New(type.modifier.codeName));

  //============================TODO: Not here:-)
  mitk::IDICOMTagsOfInterest *toiService = nullptr;

  std::vector<us::ServiceReference<mitk::IDICOMTagsOfInterest>> toiRegisters =
    us::GetModuleContext()->GetServiceReferences<mitk::IDICOMTagsOfInterest>();
  if (!toiRegisters.empty())
  {
    if (toiRegisters.size() > 1)
      MITK_WARN << "Multiple DICOM tags of interest services found. Using just one.";
    toiService = us::GetModuleContext()->GetService<mitk::IDICOMTagsOfInterest>(toiRegisters.front());
  }

  if (toiService != nullptr)
  {
    toiService->AddTagOfInterest(segmentSequencePath);

    toiService->AddTagOfInterest(segmentNumberPath);
    toiService->AddTagOfInterest(segmentLabelPath);
    toiService->AddTagOfInterest(segmentAlgorithmTypePath);

    toiService->AddTagOfInterest(segmentSegmentedPropertyCategorySequencePath);
    toiService->AddTagOfInterest(segmentCategoryCodeValuePath);
    toiService->AddTagOfInterest(segmentCategoryCodeSchemePath);
    toiService->AddTagOfInterest(segmentCategoryCodeMeaningPath);

    toiService->AddTagOfInterest(segmentSegmentedPropertyTypeSequencePath);
    toiService->AddTagOfInterest(segmentTypeCodeValuePath);
    toiService->AddTagOfInterest(segmentTypeCodeSchemePath);
    toiService->AddTagOfInterest(segmentTypeCodeMeaningPath);

    toiService->AddTagOfInterest(segmentSegmentedPropertyModifierSequencePath);
    toiService->AddTagOfInterest(segmentModifierCodeValuePath);
    toiService->AddTagOfInterest(segmentModifierCodeSchemePath);
    toiService->AddTagOfInterest(segmentModifierCodeMeaningPath);
  }

  //============================
  // Check if original image is a DICOM image; if so, store relevant DICOM Tags into the PropertyList of new
  // segmentation image
  bool parentIsDICOM = false;

  for (const auto &element : *(original->GetPropertyList()->GetMap()))
  {
    if (element.first.find("DICOM") == 0)
    {
      parentIsDICOM = true;
      break;
    }
  }

  if (!parentIsDICOM)
    return;

  //====== Patient information ======

  // Add DICOM Tag (0010,0010) patient's name; default "No Name"
  this->SetReferenceDICOMProperty(original, segmentation, DICOMTag(0x0010, 0x0010), "NO NAME");
  // Add DICOM Tag (0010,0020) patient id; default "No Name"
  this->SetReferenceDICOMProperty(original, segmentation, DICOMTag(0x0010, 0x0020), "NO NAME");
  // Add DICOM Tag (0010,0030) patient's birth date; no default
  this->SetReferenceDICOMProperty(original, segmentation, DICOMTag(0x0010, 0x0030));
  // Add DICOM Tag (0010,0040) patient's sex; default "U" (Unknown)
  this->SetReferenceDICOMProperty(original, segmentation, DICOMTag(0x0010, 0x0040), "U");

  //====== General study ======

  // Add DICOM Tag (0020,000D) Study Instance UID; no default --> MANDATORY!
  this->SetReferenceDICOMProperty(original, segmentation, DICOMTag(0x0020, 0x000D));
  // Add DICOM Tag (0080,0020) Study Date; no default (think about "today")
  this->SetReferenceDICOMProperty(original, segmentation, DICOMTag(0x0080, 0x0020));
  // Add DICOM Tag (0008,0050) Accession Number; no default
  this->SetReferenceDICOMProperty(original, segmentation, DICOMTag(0x0008, 0x0050));
  // Add DICOM Tag (0008,1030) Study Description; no default
  this->SetReferenceDICOMProperty(original, segmentation, DICOMTag(0x0008, 0x1030));

  //====== Reference DICOM data ======

  // Add reference file paths to referenced DICOM data
  BaseProperty::Pointer dcmFilesProp = original->GetProperty("files");
  if (dcmFilesProp.IsNotNull())
    segmentation->SetProperty("files", dcmFilesProp);
}

void mitk::Tool::SetReferenceDICOMProperty(Image *original,
                                           Image *segmentation,
                                           const DICOMTag &tag,
                                           const std::string &defaultString)
{
  std::string tagString = GeneratePropertyNameForDICOMTag(tag.GetGroup(), tag.GetElement());

  // Get DICOM property from referenced image
  BaseProperty::Pointer originalProperty = original->GetProperty(tagString.c_str());

  // if property exists, copy the informtaion to the segmentation
  if (originalProperty.IsNotNull())
    segmentation->SetProperty(tagString.c_str(), originalProperty);
  else // use the default value, if there is one
  {
    if (!defaultString.empty())
      segmentation->SetProperty(tagString.c_str(), StringProperty::New(defaultString).GetPointer());
  }
}

us::ModuleResource mitk::Tool::GetIconResource() const
{
  // Each specific tool should load its own resource. This one will be invalid
  return us::ModuleResource();
}

us::ModuleResource mitk::Tool::GetCursorIconResource() const
{
  // Each specific tool should load its own resource. This one will be invalid
  return us::ModuleResource();
}
