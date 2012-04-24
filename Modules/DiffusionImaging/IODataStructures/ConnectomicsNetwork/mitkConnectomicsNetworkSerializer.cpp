/*=========================================================================
 
Program:   Medical Imaging & Interaction Toolkit
Language:  C++
Date:      $Date$
Version:   $Revision: 1.12 $
 
Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.
 
This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.
 
=========================================================================*/

#include "mitkConnectomicsNetworkSerializer.h"
#include "mitkConnectomicsNetwork.h"
#include "mitkConnectomicsNetworkWriter.h"

#include <itksys/SystemTools.hxx>


MITK_REGISTER_SERIALIZER(ConnectomicsNetworkSerializer)


mitk::ConnectomicsNetworkSerializer::ConnectomicsNetworkSerializer()
{
}


mitk::ConnectomicsNetworkSerializer::~ConnectomicsNetworkSerializer()
{
}


std::string mitk::ConnectomicsNetworkSerializer::Serialize()
{
  const ConnectomicsNetwork* conNet = dynamic_cast<const ConnectomicsNetwork*>( m_Data.GetPointer() );
  if (conNet == NULL)
  {
    MITK_ERROR << " Object at " << (const void*) this->m_Data
              << " is not an mitk::ConnectomicsNetwork. Cannot serialize as ConnectomicsNetwork.";
    return "";
  }

  std::string filename( this->GetUniqueFilenameInWorkingDirectory() );
  filename += "_";
  filename += m_FilenameHint;
  filename += ".cnf";

  std::string fullname(m_WorkingDirectory);
  fullname += "/";
  fullname += itksys::SystemTools::ConvertToOutputPath(filename.c_str());

  try
  {
    ConnectomicsNetworkWriter::Pointer writer = ConnectomicsNetworkWriter::New();
    writer->SetFileName(fullname);
    writer->SetInputConnectomicsNetwork(const_cast<ConnectomicsNetwork*>(conNet));
    writer->Write();
  }
  catch (std::exception& e)
  {
    MITK_ERROR << " Error serializing object at " << (const void*) this->m_Data
              << " to " 
              << fullname 
              << ": " 
              << e.what();
    return "";
  }
  return filename;
}

