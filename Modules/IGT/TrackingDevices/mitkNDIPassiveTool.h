/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/


#ifndef MITKNDIPASSIVETOOL_H_HEADER_INCLUDED_
#define MITKNDIPASSIVETOOL_H_HEADER_INCLUDED_

#include <mitkTrackingTool.h>
#include "mitkTrackingTypes.h"

namespace mitk
{
  class NDITrackingDevice;
  class LancetVegaTrackingDevice;
  /**Documentation
  * \brief Implementation of a passive NDI optical tool
  *
  * implements the TrackingTool interface and has the ability to
  * load an srom file that contains the marker configuration for that tool
  *
  * \ingroup IGT
  */
  class MITKIGT_EXPORT NDIPassiveTool : public TrackingTool
  {
  public:
    friend class NDITrackingDevice;
    friend class LancetVegaTrackingDevice;
    /**
    * \brief tracking priority for NDI tracking devices
    */
    enum TrackingPriority
    {
      Static    = 'S',
      Dynamic   = 'D',
      ButtonBox = 'B'
    };

    mitkClassMacro(NDIPassiveTool, TrackingTool);


    virtual bool LoadSROMFile(const char* filename);      ///< load a srom tool description file
    virtual const unsigned char* GetSROMData() const;     ///< get loaded srom file as unsigned char array
    virtual unsigned int GetSROMDataLength() const;       ///< get length of SROMData char array

    itkSetStringMacro(PortHandle);    ///< get port handle under which the tool is registered in the tracking device
    itkGetStringMacro(PortHandle);    ///< set port handle under which the tool is registered in the tracking device
    itkSetMacro(TrackingPriority, TrackingPriority);      ///< set tracking priority that the ndi tracking device should use
    itkGetConstMacro(TrackingPriority, TrackingPriority); ///< get tracking priority that the ndi tracking device should use
    itkSetStringMacro(SerialNumber);  ///< set serial number of the tool
    itkGetStringMacro(SerialNumber);  ///< get serial number of the tool
    itkGetStringMacro(File);          ///< get file from which this tool was loaded
    itkSetMacro(FrameNumber,uint32_t);
    itkGetMacro(FrameNumber, uint32_t);
  protected:
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self)
    NDIPassiveTool();
    ~NDIPassiveTool() override;

    uint32_t m_FrameNumber;

    unsigned char* m_SROMData;            ///< content of the srom tool description file
    unsigned int m_SROMDataLength;        ///< length of the  srom tool description file
    TrackingPriority m_TrackingPriority;  ///< priority for this tool
    std::string m_PortHandle;             ///< port handle for this tool
    std::string m_SerialNumber;           ///< serial number for this tool
    std::string m_File;                   ///< the original file from which this tool was loaded
  };
} // namespace mitk
#endif /* MITKNDIPASSIVETOOL_H_HEADER_INCLUDED_ */
