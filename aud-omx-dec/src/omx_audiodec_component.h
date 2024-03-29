/*
  Copyright (C) 2007-2009 STMicroelectronics
  Copyright (C) 2007-2009 Nokia Corporation and/or its subsidiary(-ies)

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA
  02110-1301  USA

  $Date: 2009-10-09 14:34:15 +0200 (Fri, 09 Oct 2009) $
  Revision $Rev: 875 $
  Author $Author: gsent $

  You can also contact with mail(gangx.li@intel.com)
*/

#ifndef _OMX_AUDIODEC_COMPONENT_H_
#define _OMX_AUDIODEC_COMPONENT_H_

#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <string.h>
#include <bellagio/omx_base_filter.h>

#ifndef INT64_C
#define INT64_C
#define UINT64_C
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

#define AUDIO_DEC_BASE_NAME "OMX.st.audio_decoder"
#define AUDIO_DEC_AC3_NAME "OMX.st.audio_decoder.ac3"
#define AUDIO_DEC_AC3_ROLE "audio_decoder.ac3"

// AudioDec component private structure.
DERIVEDCLASS(omx_audiodec_component_PrivateType, omx_base_filter_PrivateType)
#define omx_audiodec_component_PrivateType_FIELDS omx_base_filter_PrivateType_FIELDS \
	/** @param avCodec pointer to the ffpeg audio decoder */ \
	AVCodec *avCodec;  \
	/** @param avCodecContext pointer to FFmpeg decoder context  */ \
	AVCodecContext *avCodecContext;  \
	/** @param pAudioAC3 Reference to OMX_AUDIO_PARAM_MP3TYPE structure*/  \
	OMX_AUDIO_PARAM_AC3TYPE pAudioAc3;  \
	/** @param pAudioAc3 Reference to OMX_AUDIO_CONFIG_SAMPLEFORMATTYPE structure*/  \
	OMX_AUDIO_CONFIG_SAMPLEFORMATTYPE pAudioSampleFormat;  \
	/** @param pPortParam Reference to OMX_PARAM_PORTDEFINITIONTYPE structure*/  \
	OMX_PARAM_PORTDEFINITIONTYPE* pPortParam;  \
	/** @param pAudioPcmMode Reference to OMX_AUDIO_PARAM_PCMMODETYPE structure*/  \
	OMX_AUDIO_PARAM_PCMMODETYPE pAudioPcmMode;  \
	/** @param avcodecReady boolean flag that is true when the audio coded has been initialized */ \
	OMX_BOOL avcodecReady;  \
	/** @param minBufferLength Field that stores the minimum allowed size for FFmpeg decoder */ \
	OMX_U16 minBufferLength; \
	/** @param inputCurrBuffer Field that stores pointer of the current input buffer position */ \
	OMX_U8* inputCurrBuffer;\
	/** @param inputCurrLength Field that stores current input buffer length in bytes */ \
	OMX_U32 inputCurrLength;\
	/** @param internalOutputBuffer Field used for first internal output buffer */ \
	OMX_U8* internalOutputBuffer;\
	/** @param isFirstBuffer Field that the buffer is the first buffer */ \
	OMX_S32 isFirstBuffer;\
	/** @param positionInOutBuf Field that used to calculate starting address of the next output frame to be written */ \
	OMX_S32 positionInOutBuf; \
	/** @param isNewBuffer Field that indicate a new buffer has arrived*/ \
	OMX_S32 isNewBuffer;  \
	/** @param audio_coding_type Field that indicate the supported audio format of audio decoder */ \
	OMX_U32 audio_coding_type;   \
	/** @param extradata pointer to extradata*/ \
	OMX_U8* extradata; \
	/** @param extradata_size extradata size*/ \
	OMX_U32 extradata_size;
	ENDCLASS(omx_audiodec_component_PrivateType)

// Component private entry points declaration
OMX_ERRORTYPE omx_audiodec_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp,OMX_STRING cComponentName);
OMX_ERRORTYPE omx_audiodec_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE omx_audiodec_component_Init(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE omx_audiodec_component_Deinit(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE omx_audiodec_component_MessageHandler(OMX_COMPONENTTYPE*,internalRequestMessageType*);

void omx_audiodec_component_BufferMgmtCallback(
	OMX_COMPONENTTYPE *openmaxStandComp,
	OMX_BUFFERHEADERTYPE* inputbuffer,
	OMX_BUFFERHEADERTYPE* outputbuffer);

OMX_ERRORTYPE omx_audiodec_component_GetParameter(
	OMX_HANDLETYPE hComponent,
	OMX_INDEXTYPE nParamIndex,
	OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE omx_audiodec_component_SetParameter(
	OMX_HANDLETYPE hComponent,
	OMX_INDEXTYPE nParamIndex,
	OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE omx_audiodec_component_ComponentRoleEnum(
	OMX_HANDLETYPE hComponent,
	OMX_U8 *cRole,
	OMX_U32 nIndex);

void omx_audiodec_component_SetInternalParameters(OMX_COMPONENTTYPE *openmaxStandComp);

OMX_ERRORTYPE omx_audiodec_component_SetConfig(
	OMX_HANDLETYPE hComponent,
	OMX_INDEXTYPE nIndex,
	OMX_PTR pComponentConfigStructure);

#endif
