#ifndef IMAGETIMESELECTOR_H_HEADER_INCLUDED_C1E4861D
#define IMAGETIMESELECTOR_H_HEADER_INCLUDED_C1E4861D

#include "mitkCommon.h"
#include "ImageToImageFilter.h"
#include "SubImageSelector.h"

namespace mitk {

//##ModelId=3DCBFE4C006F
class ImageTimeSelector : public SubImageSelector
{
public:
	/** Standard class typedefs. */
	//##ModelId=3E18A0140171
	typedef ImageTimeSelector        Self;
	//##ModelId=3E18A01401B7
	typedef SubImageSelector         Superclass;
	//##ModelId=3E18A01401FD
	typedef itk::SmartPointer<Self>  Pointer;
	//##ModelId=3E18A0140243
	typedef itk::SmartPointer<const Self>  ConstPointer;

	/** Run-time type information (and related methods). */
	itkTypeMacro(ImageTimeSelector,SubImageSelector);

	itkNewMacro(Self);  

	itkGetConstMacro(TimeNr,int);
	itkSetMacro(TimeNr,int);

	itkGetConstMacro(ChannelNr,int);
	itkSetMacro(ChannelNr,int);

	//##ModelId=3E1B1975031E
	ImageTimeSelector();
	//##ModelId=3E1B1975033C
	virtual ~ImageTimeSelector();

protected:
	//##ModelId=3E1B1A0501C7
	int m_TimeNr;

	//##ModelId=3E1B1A08024D
	int m_ChannelNr;


};

} // namespace mitk



#endif /* IMAGETIMESELECTOR_H_HEADER_INCLUDED_C1E4861D */
