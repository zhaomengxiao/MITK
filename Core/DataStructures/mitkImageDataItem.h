#ifndef IMAGEDATAITEM_H_HEADER_INCLUDED_C1F4DAB4
#define IMAGEDATAITEM_H_HEADER_INCLUDED_C1F4DAB4

#include "mitkCommon.h"
#include "PixelType.h"

namespace mitk {

//##ModelId=3D7B424502D7
class ImageDataItem : public itk::LightObject
{
  public:
    //##ModelId=3E0B6F06023F
    typedef ImageDataItem Self;

    //##ModelId=3E0B6F060290
    typedef itk::LightObject Superclass;

    //##ModelId=3E0B6F0602D6
    typedef itk::SmartPointer<Self> Pointer;

    //##ModelId=3E0B6F060308
    typedef itk::SmartPointer<const Self> ConstPointer;

    //##ModelId=3E0B7133029B
    itk::DataObject::Pointer m_itkImageObject;

    //##ModelId=3E0B7882024B
	ImageDataItem(const ImageDataItem& aParent, unsigned int dimension, int offset = 0);

    //##ModelId=3E0B78820287
	~ImageDataItem();
    //##ModelId=3E159C240213
    ImageDataItem(const mitk::PixelType& type, unsigned int dimension, unsigned int *dimensions);
    //##ModelId=3E19F8F00098
    void* GetData() const
    {
        return m_Data;
    }
    //##ModelId=3E19F8F00188
    bool IsComplete() const
    {
        return m_IsComplete;
    }
    //##ModelId=3E1A02A10244
    void SetComplete(bool complete)
    {
        m_IsComplete = complete;
    }
    //##ModelId=3E19F8F0025A
    int GetOffset() const
    {
        return m_Offset;
    }
    //##ModelId=3E19F8F0032C
    ImageDataItem::ConstPointer GetParent() const
    {
        return m_Parent;
    }
    //##ModelId=3E19F8F1002B
    ipPicDescriptor* GetPicDescriptor() const
    {
        return m_PicDescriptor;
    }
    //##ModelId=3E19F8F10111
    vtkImageData* GetVtkImageData() const
    {
        return m_VtkImageData;
    }
  protected:
    //##ModelId=3D7B42E90201
    void* m_Data;
    //##ModelId=3E0B487201C0
    ipPicDescriptor* m_PicDescriptor;
    //##ModelId=3E0B488C0380
    vtkImageData* m_VtkImageData;
    //##ModelId=3E0B89080207
	int m_Offset;
    //##ModelId=3E156DC500D4
	bool m_IsComplete;


  private:
    //##ModelId=3D7B425E0337
    ImageDataItem::ConstPointer m_Parent;

};

} // namespace mitk



#endif /* IMAGEDATAITEM_H_HEADER_INCLUDED_C1F4DAB4 */
