/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                         CCCC  M   M  Y   Y  K   K                           %
%                        C      MM MM   Y Y   K  K                            %
%                        C      M M M    Y    KKK                             %
%                        C      M   M    Y    K  K                            %
%                         CCCC  M   M    Y    K   K                           %
%                                                                             %
%                                                                             %
%                     Read/Write RAW CMYK Image Format                        %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1992                                   %
%                                                                             %
%                                                                             %
%  Copyright 1999-2020 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    https://imagemagick.org/script/license.php                               %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/channel.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/constitute.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/static.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/module.h"
#include "MagickCore/utility.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteCMYKImage(const ImageInfo *,Image *,ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d C M Y K I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadCMYKImage() reads an image of raw CMYK or CMYKA samples and returns it.
%  It allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadCMYKImage method is:
%
%      Image *ReadCMYKImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *ReadCMYKImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  const void
    *stream;

  Image
    *canvas_image,
    *image;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  QuantumInfo
    *quantum_info;

  QuantumType
    quantum_type;

  ssize_t
    i;

  size_t
    length;

  ssize_t
    count,
    y;

  unsigned char
    *pixels;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  image=AcquireImage(image_info,exception);
  if ((image->columns == 0) || (image->rows == 0))
    ThrowReaderException(OptionError,"MustSpecifyImageSize");
  status=SetImageExtent(image,image->columns,image->rows,exception);
  if (status == MagickFalse)
    return(DestroyImageList(image));
  (void) SetImageColorspace(image,CMYKColorspace,exception);
  if (image_info->interlace != PartitionInterlace)
    {
      status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
      if (status == MagickFalse)
        {
          image=DestroyImageList(image);
          return((Image *) NULL);
        }
      if (DiscardBlobBytes(image,(MagickSizeType) image->offset) == MagickFalse)
        ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
          image->filename);
    }
  /*
    Create virtual canvas to support cropping (i.e. image.cmyk[100x100+10+20]).
  */
  canvas_image=CloneImage(image,image->extract_info.width,1,MagickFalse,
    exception);
  if (canvas_image == (Image *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  (void) SetImageVirtualPixelMethod(canvas_image,BlackVirtualPixelMethod,
    exception);
  quantum_info=AcquireQuantumInfo(image_info,canvas_image);
  if (quantum_info == (QuantumInfo *) NULL)
    {
      canvas_image=DestroyImage(canvas_image);
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    }
  quantum_type=CMYKQuantum;
  if (LocaleCompare(image_info->magick,"CMYKA") == 0)
    {
      quantum_type=CMYKAQuantum;
      image->alpha_trait=BlendPixelTrait;
    }
  pixels=GetQuantumPixels(quantum_info);
  if (image_info->number_scenes != 0)
    while (image->scene < image_info->scene)
    {
      /*
        Skip to next image.
      */
      image->scene++;
      length=GetQuantumExtent(canvas_image,quantum_info,quantum_type);
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        stream=ReadBlobStream(image,length,pixels,&count);
        if (count != (ssize_t) length)
          break;
      }
    }
  count=0;
  length=0;
  scene=0;
  status=MagickTrue;
  stream=NULL;
  do
  {
    /*
      Read pixels to virtual canvas image then push to image.
    */
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    status=SetImageExtent(image,image->columns,image->rows,exception);
    if (status == MagickFalse)
      break;
    if (SetImageColorspace(image,CMYKColorspace,exception) == MagickFalse)
      break;
    switch (image_info->interlace)
    {
      case NoInterlace:
      default:
      {
        /*
          No interlacing:  CMYKCMYKCMYKCMYKCMYKCMYK...
        */
        if (scene == 0)
          {
            length=GetQuantumExtent(canvas_image,quantum_info,quantum_type);
            stream=ReadBlobStream(image,length,pixels,&count);
          }
        for (y=0; y < (ssize_t) image->extract_info.height; y++)
        {
          const Quantum
            *magick_restrict p;

          Quantum
            *magick_restrict q;

          ssize_t
            x;

          if (count != (ssize_t) length)
            {
              status=MagickFalse;
              ThrowFileException(exception,CorruptImageError,
                "UnexpectedEndOfFile",image->filename);
              break;
            }
          q=GetAuthenticPixels(canvas_image,0,0,canvas_image->columns,1,
            exception);
          if (q == (Quantum *) NULL)
            break;
          length=ImportQuantumPixels(canvas_image,(CacheView *) NULL,
            quantum_info,quantum_type,(unsigned char *) stream,exception);
          if (SyncAuthenticPixels(canvas_image,exception) == MagickFalse)
            break;
          if (((y-image->extract_info.y) >= 0) && 
              ((y-image->extract_info.y) < (ssize_t) image->rows))
            {
              p=GetVirtualPixels(canvas_image,canvas_image->extract_info.x,0,
                canvas_image->columns,1,exception);
              q=QueueAuthenticPixels(image,0,y-image->extract_info.y,
                image->columns,1,exception);
              if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
                break;
              for (x=0; x < (ssize_t) image->columns; x++)
              {
                SetPixelRed(image,GetPixelRed(canvas_image,p),q);
                SetPixelGreen(image,GetPixelGreen(canvas_image,p),q);
                SetPixelBlue(image,GetPixelBlue(canvas_image,p),q);
                SetPixelBlack(image,GetPixelBlack(canvas_image,p),q);
                SetPixelAlpha(image,OpaqueAlpha,q);
                if (image->alpha_trait != UndefinedPixelTrait)
                  SetPixelAlpha(image,GetPixelAlpha(canvas_image,p),q);
                p+=GetPixelChannels(canvas_image);
                q+=GetPixelChannels(image);
              }
              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
            }
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,LoadImageTag,(MagickOffsetType) y,
                image->rows);
              if (status == MagickFalse)
                break;
            }
          stream=ReadBlobStream(image,length,pixels,&count);
        }
        break;
      }
      case LineInterlace:
      {
        static QuantumType
          quantum_types[5] =
          {
            CyanQuantum,
            MagentaQuantum,
            YellowQuantum,
            BlackQuantum,
            OpacityQuantum
          };

        /*
          Line interlacing:  CCC...MMM...YYY...KKK...CCC...MMM...YYY...KKK...
        */
        if (scene == 0)
          {
            length=GetQuantumExtent(canvas_image,quantum_info,CyanQuantum);
            stream=ReadBlobStream(image,length,pixels,&count);
          }
        for (y=0; y < (ssize_t) image->extract_info.height; y++)
        {
          for (i=0; i < (ssize_t) (image->alpha_trait != UndefinedPixelTrait ? 5 : 4); i++)
          {
            const Quantum
              *magick_restrict p;

            Quantum
              *magick_restrict q;

            ssize_t
              x;

            if (count != (ssize_t) length)
              {
                status=MagickFalse;
                ThrowFileException(exception,CorruptImageError,
                  "UnexpectedEndOfFile",image->filename);
                break;
              }
            quantum_type=quantum_types[i];
            q=GetAuthenticPixels(canvas_image,0,0,canvas_image->columns,1,
              exception);
            if (q == (Quantum *) NULL)
              break;
            length=ImportQuantumPixels(canvas_image,(CacheView *) NULL,
              quantum_info,quantum_type,(unsigned char *) stream,exception);
            if (SyncAuthenticPixels(canvas_image,exception) == MagickFalse)
              break;
            if (((y-image->extract_info.y) >= 0) && 
                ((y-image->extract_info.y) < (ssize_t) image->rows))
              {
                p=GetVirtualPixels(canvas_image,canvas_image->extract_info.x,
                  0,canvas_image->columns,1,exception);
                q=GetAuthenticPixels(image,0,y-image->extract_info.y,
                  image->columns,1,exception);
                if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
                  break;
                for (x=0; x < (ssize_t) image->columns; x++)
                {
                  switch (quantum_type)
                  {
                    case CyanQuantum:
                    {
                      SetPixelCyan(image,GetPixelCyan(canvas_image,p),q);
                      break;
                    }
                    case MagentaQuantum:
                    {
                      SetPixelMagenta(image,GetPixelMagenta(canvas_image,p),q);
                      break;
                    }
                    case YellowQuantum:
                    {
                      SetPixelYellow(image,GetPixelYellow(canvas_image,p),q);
                      break;
                    }
                    case BlackQuantum:
                    {
                      SetPixelBlack(image,GetPixelBlack(canvas_image,p),q);
                      break;
                    }
                    case OpacityQuantum:
                    {
                      SetPixelAlpha(image,GetPixelAlpha(canvas_image,p),q);
                      break;
                    }
                    default:
                      break;
                  }
                  p+=GetPixelChannels(canvas_image);
                  q+=GetPixelChannels(image);
                }
                if (SyncAuthenticPixels(image,exception) == MagickFalse)
                  break;
              }
            stream=ReadBlobStream(image,length,pixels,&count);
          }
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,LoadImageTag,(MagickOffsetType) y,
                image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        break;
      }
      case PlaneInterlace:
      {
        /*
          Plane interlacing:  CCCCCC...MMMMMM...YYYYYY...KKKKKK...
        */
        if (scene == 0)
          {
            length=GetQuantumExtent(canvas_image,quantum_info,CyanQuantum);
            stream=ReadBlobStream(image,length,pixels,&count);
          }
        for (y=0; y < (ssize_t) image->extract_info.height; y++)
        {
          const Quantum
            *magick_restrict p;

          Quantum
            *magick_restrict q;

          ssize_t
            x;

          if (count != (ssize_t) length)
            {
              status=MagickFalse;
              ThrowFileException(exception,CorruptImageError,
                "UnexpectedEndOfFile",image->filename);
              break;
            }
          q=GetAuthenticPixels(canvas_image,0,0,canvas_image->columns,1,
            exception);
          if (q == (Quantum *) NULL)
            break;
          length=ImportQuantumPixels(canvas_image,(CacheView *) NULL,
            quantum_info,CyanQuantum,(unsigned char *) stream,exception);
          if (SyncAuthenticPixels(canvas_image,exception) == MagickFalse)
            break;
          if (((y-image->extract_info.y) >= 0) && 
              ((y-image->extract_info.y) < (ssize_t) image->rows))
            {
              p=GetVirtualPixels(canvas_image,canvas_image->extract_info.x,0,
                canvas_image->columns,1,exception);
              q=GetAuthenticPixels(image,0,y-image->extract_info.y,
                image->columns,1,exception);
              if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
                break;
              for (x=0; x < (ssize_t) image->columns; x++)
              {
                SetPixelRed(image,GetPixelRed(canvas_image,p),q);
                p+=GetPixelChannels(canvas_image);
                q+=GetPixelChannels(image);
              }
              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
            }
          stream=ReadBlobStream(image,length,pixels,&count);
        }
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,LoadImageTag,1,6);
            if (status == MagickFalse)
              break;
          }
        for (y=0; y < (ssize_t) image->extract_info.height; y++)
        {
          const Quantum
            *magick_restrict p;

          Quantum
            *magick_restrict q;

          ssize_t
            x;

          if (count != (ssize_t) length)
            {
              status=MagickFalse;
              ThrowFileException(exception,CorruptImageError,
                "UnexpectedEndOfFile",image->filename);
              break;
            }
          q=GetAuthenticPixels(canvas_image,0,0,canvas_image->columns,1,
            exception);
          if (q == (Quantum *) NULL)
            break;
          length=ImportQuantumPixels(canvas_image,(CacheView *) NULL,
            quantum_info,MagentaQuantum,(unsigned char *) stream,exception);
          if (SyncAuthenticPixels(canvas_image,exception) == MagickFalse)
            break;
          if (((y-image->extract_info.y) >= 0) && 
              ((y-image->extract_info.y) < (ssize_t) image->rows))
            {
              p=GetVirtualPixels(canvas_image,canvas_image->extract_info.x,0,
                canvas_image->columns,1,exception);
              q=GetAuthenticPixels(image,0,y-image->extract_info.y,
                image->columns,1,exception);
              if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
                break;
              for (x=0; x < (ssize_t) image->columns; x++)
              {
                SetPixelGreen(image,GetPixelGreen(canvas_image,p),q);
                p+=GetPixelChannels(canvas_image);
                q+=GetPixelChannels(image);
              }
              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
           }
          stream=ReadBlobStream(image,length,pixels,&count);
        }
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,LoadImageTag,2,6);
            if (status == MagickFalse)
              break;
          }
        for (y=0; y < (ssize_t) image->extract_info.height; y++)
        {
          const Quantum
            *magick_restrict p;

          Quantum
            *magick_restrict q;

          ssize_t
            x;

          if (count != (ssize_t) length)
            {
              status=MagickFalse;
              ThrowFileException(exception,CorruptImageError,
                "UnexpectedEndOfFile",image->filename);
              break;
            }
          q=GetAuthenticPixels(canvas_image,0,0,canvas_image->columns,1,
            exception);
          if (q == (Quantum *) NULL)
            break;
          length=ImportQuantumPixels(canvas_image,(CacheView *) NULL,
            quantum_info,YellowQuantum,(unsigned char *) stream,exception);
          if (SyncAuthenticPixels(canvas_image,exception) == MagickFalse)
            break;
          if (((y-image->extract_info.y) >= 0) && 
              ((y-image->extract_info.y) < (ssize_t) image->rows))
            {
              p=GetVirtualPixels(canvas_image,canvas_image->extract_info.x,0,
                canvas_image->columns,1,exception);
              q=GetAuthenticPixels(image,0,y-image->extract_info.y,
                image->columns,1,exception);
              if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
                break;
              for (x=0; x < (ssize_t) image->columns; x++)
              {
                SetPixelBlue(image,GetPixelBlue(canvas_image,p),q);
                p+=GetPixelChannels(canvas_image);
                q+=GetPixelChannels(image);
              }
              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
            }
          stream=ReadBlobStream(image,length,pixels,&count);
        }
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,LoadImageTag,3,6);
            if (status == MagickFalse)
              break;
          }
        for (y=0; y < (ssize_t) image->extract_info.height; y++)
        {
          const Quantum
            *magick_restrict p;

          Quantum
            *magick_restrict q;

          ssize_t
            x;

          if (count != (ssize_t) length)
            {
              status=MagickFalse;
              ThrowFileException(exception,CorruptImageError,
                "UnexpectedEndOfFile",image->filename);
              break;
            }
          q=GetAuthenticPixels(canvas_image,0,0,canvas_image->columns,1,
            exception);
          if (q == (Quantum *) NULL)
            break;
          length=ImportQuantumPixels(canvas_image,(CacheView *) NULL,
            quantum_info,BlackQuantum,(unsigned char *) stream,exception);
          if (SyncAuthenticPixels(canvas_image,exception) == MagickFalse)
            break;
          if (((y-image->extract_info.y) >= 0) && 
              ((y-image->extract_info.y) < (ssize_t) image->rows))
            {
              p=GetVirtualPixels(canvas_image,canvas_image->extract_info.x,0,
                canvas_image->columns,1,exception);
              q=GetAuthenticPixels(image,0,y-image->extract_info.y,
                image->columns,1,exception);
              if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
                break;
              for (x=0; x < (ssize_t) image->columns; x++)
              {
                SetPixelBlack(image,GetPixelBlack(canvas_image,p),q);
                p+=GetPixelChannels(canvas_image);
                q+=GetPixelChannels(image);
              }
              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
            }
          stream=ReadBlobStream(image,length,pixels,&count);
        }
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,LoadImageTag,4,6);
            if (status == MagickFalse)
              break;
          }
        if (image->alpha_trait != UndefinedPixelTrait)
          {
            for (y=0; y < (ssize_t) image->extract_info.height; y++)
            {
              const Quantum
                *magick_restrict p;

              Quantum
                *magick_restrict q;

              ssize_t
                x;

              if (count != (ssize_t) length)
                {
                  status=MagickFalse;
                  ThrowFileException(exception,CorruptImageError,
                    "UnexpectedEndOfFile",image->filename);
                  break;
                }
              q=GetAuthenticPixels(canvas_image,0,0,canvas_image->columns,1,
                exception);
              if (q == (Quantum *) NULL)
                break;
              length=ImportQuantumPixels(canvas_image,(CacheView *) NULL,
                quantum_info,AlphaQuantum,(unsigned char *) stream,exception);
              if (SyncAuthenticPixels(canvas_image,exception) == MagickFalse)
                break;
              if (((y-image->extract_info.y) >= 0) && 
                  ((y-image->extract_info.y) < (ssize_t) image->rows))
                {
                  p=GetVirtualPixels(canvas_image,
                    canvas_image->extract_info.x,0,canvas_image->columns,1,
                    exception);
                  q=GetAuthenticPixels(image,0,y-image->extract_info.y,
                    image->columns,1,exception);
                  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
                    break;
                  for (x=0; x < (ssize_t) image->columns; x++)
                  {
                    SetPixelAlpha(image,GetPixelAlpha(canvas_image,p),q);
                    p+=GetPixelChannels(canvas_image);
                    q+=GetPixelChannels(image);
                  }
                  if (SyncAuthenticPixels(image,exception) == MagickFalse)
                    break;
                }
              stream=ReadBlobStream(image,length,pixels,&count);
            }
            if (image->previous == (Image *) NULL)
              {
                status=SetImageProgress(image,LoadImageTag,5,6);
                if (status == MagickFalse)
                  break;
              }
          }
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,LoadImageTag,6,6);
            if (status == MagickFalse)
              break;
          }
        break;
      }
      case PartitionInterlace:
      {
        /*
          Partition interlacing:  CCCCCC..., MMMMMM..., YYYYYY..., KKKKKK...
        */
        AppendImageFormat("C",image->filename);
        status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
        if (status == MagickFalse)
          break;
        if (DiscardBlobBytes(image,(MagickSizeType) image->offset) == MagickFalse)
          {
            status=MagickFalse;
            ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
              image->filename);
            break;
          }
        length=GetQuantumExtent(canvas_image,quantum_info,CyanQuantum);
        for (i=0; i < (ssize_t) scene; i++)
        {
          for (y=0; y < (ssize_t) image->extract_info.height; y++)
          {
            stream=ReadBlobStream(image,length,pixels,&count);
            if (count != (ssize_t) length)
              break;
          }
          if (count != (ssize_t) length)
            break;
        }
        stream=ReadBlobStream(image,length,pixels,&count);
        for (y=0; y < (ssize_t) image->extract_info.height; y++)
        {
          const Quantum
            *magick_restrict p;

          Quantum
            *magick_restrict q;

          ssize_t
            x;

          if (count != (ssize_t) length)
            {
              status=MagickFalse;
              ThrowFileException(exception,CorruptImageError,
                "UnexpectedEndOfFile",image->filename);
              break;
            }
          q=GetAuthenticPixels(canvas_image,0,0,canvas_image->columns,1,
            exception);
          if (q == (Quantum *) NULL)
            break;
          length=ImportQuantumPixels(canvas_image,(CacheView *) NULL,
            quantum_info,CyanQuantum,(unsigned char *) stream,exception);
          if (SyncAuthenticPixels(canvas_image,exception) == MagickFalse)
            break;
          if (((y-image->extract_info.y) >= 0) && 
              ((y-image->extract_info.y) < (ssize_t) image->rows))
            {
              p=GetVirtualPixels(canvas_image,canvas_image->extract_info.x,0,
                canvas_image->columns,1,exception);
              q=GetAuthenticPixels(image,0,y-image->extract_info.y,
                image->columns,1,exception);
              if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
                break;
              for (x=0; x < (ssize_t) image->columns; x++)
              {
                SetPixelRed(image,GetPixelRed(canvas_image,p),q);
                p+=GetPixelChannels(canvas_image);
                q+=GetPixelChannels(image);
              }
              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
            }
          stream=ReadBlobStream(image,length,pixels,&count);
        }
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,LoadImageTag,1,5);
            if (status == MagickFalse)
              break;
          }
        (void) CloseBlob(image);
        AppendImageFormat("M",image->filename);
        status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
        if (status == MagickFalse)
          break;
        length=GetQuantumExtent(canvas_image,quantum_info,MagentaQuantum);
        for (i=0; i < (ssize_t) scene; i++)
        {
          for (y=0; y < (ssize_t) image->extract_info.height; y++)
          {
            stream=ReadBlobStream(image,length,pixels,&count);
            if (count != (ssize_t) length)
              break;
          }
          if (count != (ssize_t) length)
            break;
        }
        stream=ReadBlobStream(image,length,pixels,&count);
        for (y=0; y < (ssize_t) image->extract_info.height; y++)
        {
          const Quantum
            *magick_restrict p;

          Quantum
            *magick_restrict q;

          ssize_t
            x;

          if (count != (ssize_t) length)
            {
              status=MagickFalse;
              ThrowFileException(exception,CorruptImageError,
                "UnexpectedEndOfFile",image->filename);
              break;
            }
          q=GetAuthenticPixels(canvas_image,0,0,canvas_image->columns,1,
            exception);
          if (q == (Quantum *) NULL)
            break;
          length=ImportQuantumPixels(canvas_image,(CacheView *) NULL,
            quantum_info,MagentaQuantum,(unsigned char *) stream,exception);
          if (SyncAuthenticPixels(canvas_image,exception) == MagickFalse)
            break;
          if (((y-image->extract_info.y) >= 0) && 
              ((y-image->extract_info.y) < (ssize_t) image->rows))
            {
              p=GetVirtualPixels(canvas_image,canvas_image->extract_info.x,0,
                canvas_image->columns,1,exception);
              q=GetAuthenticPixels(image,0,y-image->extract_info.y,
                image->columns,1,exception);
              if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
                break;
              for (x=0; x < (ssize_t) image->columns; x++)
              {
                SetPixelGreen(image,GetPixelGreen(canvas_image,p),q);
                p+=GetPixelChannels(canvas_image);
                q+=GetPixelChannels(image);
              }
              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
           }
          stream=ReadBlobStream(image,length,pixels,&count);
        }
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,LoadImageTag,2,5);
            if (status == MagickFalse)
              break;
          }
        (void) CloseBlob(image);
        AppendImageFormat("Y",image->filename);
        status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
        if (status == MagickFalse)
          break;
        length=GetQuantumExtent(canvas_image,quantum_info,YellowQuantum);
        for (i=0; i < (ssize_t) scene; i++)
        {
          for (y=0; y < (ssize_t) image->extract_info.height; y++)
          {
            stream=ReadBlobStream(image,length,pixels,&count);
            if (count != (ssize_t) length)
              break;
          }
          if (count != (ssize_t) length)
            break;
        }
        stream=ReadBlobStream(image,length,pixels,&count);
        for (y=0; y < (ssize_t) image->extract_info.height; y++)
        {
          const Quantum
            *magick_restrict p;

          Quantum
            *magick_restrict q;

          ssize_t
            x;

          if (count != (ssize_t) length)
            {
              status=MagickFalse;
              ThrowFileException(exception,CorruptImageError,
                "UnexpectedEndOfFile",image->filename);
              break;
            }
          q=GetAuthenticPixels(canvas_image,0,0,canvas_image->columns,1,
            exception);
          if (q == (Quantum *) NULL)
            break;
          length=ImportQuantumPixels(canvas_image,(CacheView *) NULL,
            quantum_info,YellowQuantum,(unsigned char *) stream,exception);
          if (SyncAuthenticPixels(canvas_image,exception) == MagickFalse)
            break;
          if (((y-image->extract_info.y) >= 0) && 
              ((y-image->extract_info.y) < (ssize_t) image->rows))
            {
              p=GetVirtualPixels(canvas_image,canvas_image->extract_info.x,0,
                canvas_image->columns,1,exception);
              q=GetAuthenticPixels(image,0,y-image->extract_info.y,
                image->columns,1,exception);
              if ((p == (const Quantum *) NULL) ||
                  (q == (Quantum *) NULL))
                break;
              for (x=0; x < (ssize_t) image->columns; x++)
              {
                SetPixelBlue(image,GetPixelBlue(canvas_image,p),q);
                p+=GetPixelChannels(canvas_image);
                q+=GetPixelChannels(image);
              }
              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
           }
          stream=ReadBlobStream(image,length,pixels,&count);
        }
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,LoadImageTag,3,5);
            if (status == MagickFalse)
              break;
          }
        (void) CloseBlob(image);
        AppendImageFormat("K",image->filename);
        status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
        if (status == MagickFalse)
          break;
        length=GetQuantumExtent(canvas_image,quantum_info,BlackQuantum);
        for (i=0; i < (ssize_t) scene; i++)
        {
          for (y=0; y < (ssize_t) image->extract_info.height; y++)
          {
            stream=ReadBlobStream(image,length,pixels,&count);
            if (count != (ssize_t) length)
              break;
          }
          if (count != (ssize_t) length)
            break;
        }
        stream=ReadBlobStream(image,length,pixels,&count);
        for (y=0; y < (ssize_t) image->extract_info.height; y++)
        {
          const Quantum
            *magick_restrict p;

          Quantum
            *magick_restrict q;

          ssize_t
            x;

          if (count != (ssize_t) length)
            {
              status=MagickFalse;
              ThrowFileException(exception,CorruptImageError,
                "UnexpectedEndOfFile",image->filename);
              break;
            }
          q=GetAuthenticPixels(canvas_image,0,0,canvas_image->columns,1,
            exception);
          if (q == (Quantum *) NULL)
            break;
          length=ImportQuantumPixels(canvas_image,(CacheView *) NULL,
            quantum_info,BlackQuantum,(unsigned char *) stream,exception);
          if (SyncAuthenticPixels(canvas_image,exception) == MagickFalse)
            break;
          if (((y-image->extract_info.y) >= 0) && 
              ((y-image->extract_info.y) < (ssize_t) image->rows))
            {
              p=GetVirtualPixels(canvas_image,canvas_image->extract_info.x,0,
                canvas_image->columns,1,exception);
              q=GetAuthenticPixels(image,0,y-image->extract_info.y,
                image->columns,1,exception);
              if ((p == (const Quantum *) NULL) ||
                  (q == (Quantum *) NULL))
                break;
              for (x=0; x < (ssize_t) image->columns; x++)
              {
                SetPixelBlack(image,GetPixelBlack(canvas_image,p),q);
                p+=GetPixelChannels(canvas_image);
                q+=GetPixelChannels(image);
              }
              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
           }
          stream=ReadBlobStream(image,length,pixels,&count);
        }
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,LoadImageTag,3,5);
            if (status == MagickFalse)
              break;
          }
        if (image->alpha_trait != UndefinedPixelTrait)
          {
            (void) CloseBlob(image);
            AppendImageFormat("A",image->filename);
            status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
            if (status == MagickFalse)
              break;
            length=GetQuantumExtent(canvas_image,quantum_info,AlphaQuantum);
            for (i=0; i < (ssize_t) scene; i++)
            {
              for (y=0; y < (ssize_t) image->extract_info.height; y++)
              {
                stream=ReadBlobStream(image,length,pixels,&count);
                if (count != (ssize_t) length)
                  break;
              }
              if (count != (ssize_t) length)
                break;
            }
            stream=ReadBlobStream(image,length,pixels,&count);
            for (y=0; y < (ssize_t) image->extract_info.height; y++)
            {
              const Quantum
                *magick_restrict p;

              Quantum
                *magick_restrict q;

              ssize_t
                x;

              if (count != (ssize_t) length)
                {
                  status=MagickFalse;
                  ThrowFileException(exception,CorruptImageError,
                    "UnexpectedEndOfFile",image->filename);
                  break;
                }
              q=GetAuthenticPixels(canvas_image,0,0,canvas_image->columns,1,
                exception);
              if (q == (Quantum *) NULL)
                break;
              length=ImportQuantumPixels(canvas_image,(CacheView *) NULL,
                quantum_info,YellowQuantum,(unsigned char *) stream,exception);
              if (SyncAuthenticPixels(canvas_image,exception) == MagickFalse)
                break;
              if (((y-image->extract_info.y) >= 0) && 
                  ((y-image->extract_info.y) < (ssize_t) image->rows))
                {
                  p=GetVirtualPixels(canvas_image,canvas_image->extract_info.x,
                    0,canvas_image->columns,1,exception);
                  q=GetAuthenticPixels(image,0,y-image->extract_info.y,
                    image->columns,1,exception);
                  if ((p == (const Quantum *) NULL) ||
                      (q == (Quantum *) NULL))
                    break;
                  for (x=0; x < (ssize_t) image->columns; x++)
                  {
                    SetPixelAlpha(image,GetPixelAlpha(canvas_image,p),q);
                    p+=GetPixelChannels(canvas_image);
                    q+=GetPixelChannels(image);
                  }
                  if (SyncAuthenticPixels(image,exception) == MagickFalse)
                    break;
               }
              stream=ReadBlobStream(image,length,pixels,&count);
            }
            if (image->previous == (Image *) NULL)
              {
                status=SetImageProgress(image,LoadImageTag,4,5);
                if (status == MagickFalse)
                  break;
              }
          }
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,LoadImageTag,5,5);
            if (status == MagickFalse)
              break;
          }
        break;
      }
    }
    if (status == MagickFalse)
      break;
    SetQuantumImageType(image,quantum_type);
    /*
      Proceed to next image.
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    if (count == (ssize_t) length)
      {
        /*
          Allocate next image structure.
        */
        AcquireNextImage(image_info,image,exception);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            status=MagickFalse;
            break;
          }
        image=SyncNextImageInList(image);
        status=SetImageProgress(image,LoadImagesTag,TellBlob(image),
          GetBlobSize(image));
        if (status == MagickFalse)
          break;
      }
    scene++;
  } while (count == (ssize_t) length);
  quantum_info=DestroyQuantumInfo(quantum_info);
  canvas_image=DestroyImage(canvas_image);
  (void) CloseBlob(image);
  if (status == MagickFalse)
    return(DestroyImageList(image));
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r C M Y K I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterCMYKImage() adds attributes for the CMYK image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterCMYKImage method is:
%
%      size_t RegisterCMYKImage(void)
%
*/
ModuleExport size_t RegisterCMYKImage(void)
{
  MagickInfo
    *entry;

  entry=AcquireMagickInfo("CMYK","CMYK",
    "Raw cyan, magenta, yellow, and black samples");
  entry->decoder=(DecodeImageHandler *) ReadCMYKImage;
  entry->encoder=(EncodeImageHandler *) WriteCMYKImage;
  entry->flags|=CoderRawSupportFlag;
  entry->flags|=CoderEndianSupportFlag;
  (void) RegisterMagickInfo(entry);
  entry=AcquireMagickInfo("CMYK","CMYKA",
    "Raw cyan, magenta, yellow, black, and alpha samples");
  entry->decoder=(DecodeImageHandler *) ReadCMYKImage;
  entry->encoder=(EncodeImageHandler *) WriteCMYKImage;
  entry->flags|=CoderRawSupportFlag;
  entry->flags|=CoderEndianSupportFlag;
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r C M Y K I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterCMYKImage() removes format registrations made by the
%  CMYK module from the list of supported formats.
%
%  The format of the UnregisterCMYKImage method is:
%
%      UnregisterCMYKImage(void)
%
*/
ModuleExport void UnregisterCMYKImage(void)
{
  (void) UnregisterMagickInfo("CMYK");
  (void) UnregisterMagickInfo("CMYKA");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e C M Y K I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteCMYKImage() writes an image to a file in cyan, magenta, yellow, and
%  black,rasterfile format.
%
%  The format of the WriteCMYKImage method is:
%
%      MagickBooleanType WriteCMYKImage(const ImageInfo *image_info,
%        Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image_info: the image info.
%
%    o image:  The image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType WriteCMYKImage(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  QuantumInfo
    *quantum_info;

  QuantumType
    quantum_type;

  size_t
    imageListLength,
    length;

  ssize_t
    count,
    y;

  unsigned char
    *pixels;

  /*
    Allocate memory for pixels.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image_info->interlace != PartitionInterlace)
    {
      /*
        Open output image file.
      */
      status=OpenBlob(image_info,image,WriteBinaryBlobMode,exception);
      if (status == MagickFalse)
        return(status);
    }
  scene=0;
  imageListLength=GetImageListLength(image);
  do
  {
    /*
      Convert MIFF to CMYK raster pixels.
    */
    if (image->colorspace != CMYKColorspace)
      (void) TransformImageColorspace(image,CMYKColorspace,exception);
    quantum_type=CMYKQuantum;
    if (LocaleCompare(image_info->magick,"CMYKA") == 0)
      {
        quantum_type=CMYKAQuantum;
        if (image->alpha_trait == UndefinedPixelTrait)
          (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
      }
    quantum_info=AcquireQuantumInfo(image_info,image);
    if (quantum_info == (QuantumInfo *) NULL)
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    pixels=(unsigned char *) GetQuantumPixels(quantum_info);
    switch (image_info->interlace)
    {
      case NoInterlace:
      default:
      {
        /*
          No interlacing:  CMYKCMYKCMYKCMYKCMYKCMYK...
        */
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          const Quantum
            *magick_restrict p;

          p=GetVirtualPixels(image,0,y,image->columns,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          length=ExportQuantumPixels(image,(CacheView *) NULL,quantum_info,
            quantum_type,pixels,exception);
          count=WriteBlob(image,length,pixels);
          if (count != (ssize_t) length)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,SaveImageTag,(MagickOffsetType) y,
                image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        break;
      }
      case LineInterlace:
      {
        /*
          Line interlacing:  CCC...MMM...YYY...KKK...CCC...MMM...YYY...KKK...
        */
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          const Quantum
            *magick_restrict p;

          p=GetVirtualPixels(image,0,y,image->columns,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          length=ExportQuantumPixels(image,(CacheView *) NULL,quantum_info,
            CyanQuantum,pixels,exception);
          count=WriteBlob(image,length,pixels);
          if (count != (ssize_t) length)
            break;
          length=ExportQuantumPixels(image,(CacheView *) NULL,quantum_info,
            MagentaQuantum,pixels,exception);
          count=WriteBlob(image,length,pixels);
          if (count != (ssize_t) length)
            break;
          length=ExportQuantumPixels(image,(CacheView *) NULL,quantum_info,
            YellowQuantum,pixels,exception);
          count=WriteBlob(image,length,pixels);
          if (count != (ssize_t) length)
            break;
          length=ExportQuantumPixels(image,(CacheView *) NULL,quantum_info,
            BlackQuantum,pixels,exception);
          count=WriteBlob(image,length,pixels);
          if (count != (ssize_t) length)
            break;
          if (quantum_type == CMYKAQuantum)
            {
              length=ExportQuantumPixels(image,(CacheView *) NULL,quantum_info,
                AlphaQuantum,pixels,exception);
              count=WriteBlob(image,length,pixels);
              if (count != (ssize_t) length)
                break;
            }
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,SaveImageTag,(MagickOffsetType) y,
                image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        break;
      }
      case PlaneInterlace:
      {
        /*
          Plane interlacing:  CCCCCC...MMMMMM...YYYYYY...KKKKKK...
        */
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          const Quantum
            *magick_restrict p;

          p=GetVirtualPixels(image,0,y,image->columns,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          length=ExportQuantumPixels(image,(CacheView *) NULL,quantum_info,
            CyanQuantum,pixels,exception);
          count=WriteBlob(image,length,pixels);
          if (count != (ssize_t) length)
            break;
        }
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,SaveImageTag,1,6);
            if (status == MagickFalse)
              break;
          }
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          const Quantum
            *magick_restrict p;

          p=GetVirtualPixels(image,0,y,image->columns,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          length=ExportQuantumPixels(image,(CacheView *) NULL,quantum_info,
            MagentaQuantum,pixels,exception);
          count=WriteBlob(image,length,pixels);
          if (count != (ssize_t) length)
            break;
        }
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,SaveImageTag,2,6);
            if (status == MagickFalse)
              break;
          }
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          const Quantum
            *magick_restrict p;

          p=GetVirtualPixels(image,0,y,image->columns,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          length=ExportQuantumPixels(image,(CacheView *) NULL,quantum_info,
            YellowQuantum,pixels,exception);
          count=WriteBlob(image,length,pixels);
          if (count != (ssize_t) length)
            break;
        }
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,SaveImageTag,3,6);
            if (status == MagickFalse)
              break;
          }
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          const Quantum
            *magick_restrict p;

          p=GetVirtualPixels(image,0,y,image->columns,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          length=ExportQuantumPixels(image,(CacheView *) NULL,quantum_info,
            BlackQuantum,pixels,exception);
          count=WriteBlob(image,length,pixels);
          if (count != (ssize_t) length)
            break;
        }
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,SaveImageTag,4,6);
            if (status == MagickFalse)
              break;
          }
        if (quantum_type == CMYKAQuantum)
          {
            for (y=0; y < (ssize_t) image->rows; y++)
            {
              const Quantum
                *magick_restrict p;

              p=GetVirtualPixels(image,0,y,image->columns,1,exception);
              if (p == (const Quantum *) NULL)
                break;
              length=ExportQuantumPixels(image,(CacheView *) NULL,quantum_info,
                AlphaQuantum,pixels,exception);
              count=WriteBlob(image,length,pixels);
              if (count != (ssize_t) length)
              break;
            }
            if (image->previous == (Image *) NULL)
              {
                status=SetImageProgress(image,SaveImageTag,5,6);
                if (status == MagickFalse)
                  break;
              }
          }
        if (image_info->interlace == PartitionInterlace)
          (void) CopyMagickString(image->filename,image_info->filename,
            MagickPathExtent);
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,SaveImageTag,6,6);
            if (status == MagickFalse)
              break;
          }
        break;
      }
      case PartitionInterlace:
      {
        /*
          Partition interlacing:  CCCCCC..., MMMMMM..., YYYYYY..., KKKKKK...
        */
        AppendImageFormat("C",image->filename);
        status=OpenBlob(image_info,image,scene == 0 ? WriteBinaryBlobMode :
          AppendBinaryBlobMode,exception);
        if (status == MagickFalse)
          return(status);
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          const Quantum
            *magick_restrict p;

          p=GetVirtualPixels(image,0,y,image->columns,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          length=ExportQuantumPixels(image,(CacheView *) NULL,quantum_info,
            CyanQuantum,pixels,exception);
          count=WriteBlob(image,length,pixels);
          if (count != (ssize_t) length)
            break;
        }
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,SaveImageTag,1,6);
            if (status == MagickFalse)
              break;
          }
        (void) CloseBlob(image);
        AppendImageFormat("M",image->filename);
        status=OpenBlob(image_info,image,scene == 0 ? WriteBinaryBlobMode :
          AppendBinaryBlobMode,exception);
        if (status == MagickFalse)
          return(status);
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          const Quantum
            *magick_restrict p;

          p=GetVirtualPixels(image,0,y,image->columns,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          length=ExportQuantumPixels(image,(CacheView *) NULL,quantum_info,
            MagentaQuantum,pixels,exception);
          count=WriteBlob(image,length,pixels);
          if (count != (ssize_t) length)
            break;
        }
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,SaveImageTag,2,6);
            if (status == MagickFalse)
              break;
          }
        (void) CloseBlob(image);
        AppendImageFormat("Y",image->filename);
        status=OpenBlob(image_info,image,scene == 0 ? WriteBinaryBlobMode :
          AppendBinaryBlobMode,exception);
        if (status == MagickFalse)
          return(status);
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          const Quantum
            *magick_restrict p;

          p=GetVirtualPixels(image,0,y,image->columns,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          length=ExportQuantumPixels(image,(CacheView *) NULL,quantum_info,
            YellowQuantum,pixels,exception);
          count=WriteBlob(image,length,pixels);
          if (count != (ssize_t) length)
            break;
        }
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,SaveImageTag,3,6);
            if (status == MagickFalse)
              break;
          }
        (void) CloseBlob(image);
        AppendImageFormat("K",image->filename);
        status=OpenBlob(image_info,image,scene == 0 ? WriteBinaryBlobMode :
          AppendBinaryBlobMode,exception);
        if (status == MagickFalse)
          return(status);
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          const Quantum
            *magick_restrict p;

          p=GetVirtualPixels(image,0,y,image->columns,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          length=ExportQuantumPixels(image,(CacheView *) NULL,quantum_info,
            BlackQuantum,pixels,exception);
          count=WriteBlob(image,length,pixels);
          if (count != (ssize_t) length)
            break;
        }
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,SaveImageTag,4,6);
            if (status == MagickFalse)
              break;
          }
        if (quantum_type == CMYKAQuantum)
          {
            (void) CloseBlob(image);
            AppendImageFormat("A",image->filename);
            status=OpenBlob(image_info,image,scene == 0 ? WriteBinaryBlobMode :
              AppendBinaryBlobMode,exception);
            if (status == MagickFalse)
              return(status);
            for (y=0; y < (ssize_t) image->rows; y++)
            {
              const Quantum
                *magick_restrict p;

              p=GetVirtualPixels(image,0,y,image->columns,1,exception);
              if (p == (const Quantum *) NULL)
                break;
              length=ExportQuantumPixels(image,(CacheView *) NULL,quantum_info,
                AlphaQuantum,pixels,exception);
              count=WriteBlob(image,length,pixels);
              if (count != (ssize_t) length)
                break;
            }
            if (image->previous == (Image *) NULL)
              {
                status=SetImageProgress(image,SaveImageTag,5,6);
                if (status == MagickFalse)
                  break;
              }
          }
        (void) CloseBlob(image);
        (void) CopyMagickString(image->filename,image_info->filename,
          MagickPathExtent);
        if (image->previous == (Image *) NULL)
          {
            status=SetImageProgress(image,SaveImageTag,6,6);
            if (status == MagickFalse)
              break;
          }
        break;
      }
    }
    quantum_info=DestroyQuantumInfo(quantum_info);
    if (GetNextImageInList(image) == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    status=SetImageProgress(image,SaveImagesTag,scene++,imageListLength);
    if (status == MagickFalse)
      break;
  } while (image_info->adjoin != MagickFalse);
  (void) CloseBlob(image);
  return(MagickTrue);
}
