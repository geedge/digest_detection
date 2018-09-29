#ifndef _AV_SENDBACK_H
#define _AV_SENDBACK_H

/*魔数*/
#define AV_MAGIC_VALUE				0x8739	

/*消息类型*/
#define AV_TYPE_META 				0x01
#define	AV_TYPE_DATA				0x02
#define AV_TYPE_RESULT 				0x11

/*协议类型*/
#define	PROTOCOL_DEFAULT			0x00
#define	PROTOCOL_HTTP				0x01
#define	PROTOCOL_SMTP				0x02
#define	PROTOCOL_POP3				0x03
#define	PROTOCOL_IMAP				0x04
#define	PROTOCOL_FTP				0x05
#define	PROTOCOL_HTTP_PIC			0x06    
#define	PROTOCOL_RTSP_RDT			0x07
#define	PROTOCOL_RTSP_RTP			0x08
#define	PROTOCOL_MMS				0x09
#define	PROTOCOL_RTMP				0x0A
#define	PROTOCOL_SIP				0x0B

/*媒体类型*/
#define MEDIA_TYPE_UNKNOWN    			0x00
/*媒体类型:视频支持的媒体类型*/
#define MEDIA_TYPE_VIDEO				0x01
#define MEDIA_TYPE_WMV					0x02		
#define MEDIA_TYPE_MPG					0x03		
#define MEDIA_TYPE_FLV					0x04	
#define MEDIA_TYPE_RMFF					0x05
#define MEDIA_TYPE_AVI					0x06
#define MEDIA_TYPE_SWF					0x07
#define MEDIA_TYPE_MPG4					0x08
#define MEDIA_TYPE_AIFF					0x09
#define MEDIA_TYPE_OGG					0x0A
#define MEDIA_TYPE_DRC					0x0B
#define MEDIA_TYPE_DIRECTSHOW			0x0C
#define MEDIA_TYPE_FLIC					0x0E
#define MEDIA_TYPE_INDEO				0x0F
#define MEDIA_TYPE_MKV					0x10
/*媒体类型:音频支持的媒体类型*/
#define MEDIA_TYPE_AUDIO				0x20
#define MEDIA_TYPE_MP3					0x21
/*媒体类型:图片支持的媒体类型*/
#define MEDIA_TYPE_IMAGE				0x30
#define MEDIA_TYPE_JPG					0x31
#define MEDIA_TYPE_BMP					0x32
#define MEDIA_TYPE_GIF					0x33
/*媒体类型:实时流媒体支持的媒体类型*/
#define MEDIA_TYPE_MMS   				0x40
#define MEDIA_TYPE_RTSP_RDT   			0x41
#define MEDIA_TYPE_RTSP_RTP  			0x42
/*媒体类型:碎片化音视频支持的媒体类型*/
#define MEDIA_TYPE_OSMF   				0x50
#define MEDIA_TYPE_HLS   				0x51
/*媒体类型:VOIP支持的媒体类型，亦即编码类型*/
#define MEDIA_TYPE_AUDIO_UNKNOWN		0x60
#define MEDIA_TYPE_AUDIO_G711_ULAW		0x61
#define MEDIA_TYPE_AUDIO_G711_ALAW		0x62
#define MEDIA_TYPE_AUDIO_G722			0x63
#define MEDIA_TYPE_AUDIO_G723			0x64
#define MEDIA_TYPE_AUDIO_G726_40		0x65
#define MEDIA_TYPE_AUDIO_G726_32		0x66
#define MEDIA_TYPE_AUDIO_G726_24		0x67
#define MEDIA_TYPE_AUDIO_G726_16		0x68
#define MEDIA_TYPE_AUDIO_AAL2_G726_40	0x69
#define MEDIA_TYPE_AUDIO_AAL2_G726_32	0x6A
#define MEDIA_TYPE_AUDIO_AAL2_G726_24	0x6B
#define MEDIA_TYPE_AUDIO_AAL2_G726_16	0x6C
#define MEDIA_TYPE_AUDIO_G728			0x6D
#define MEDIA_TYPE_AUDIO_G729D			0x6E
#define MEDIA_TYPE_AUDIO_G729E			0x6F
#define MEDIA_TYPE_AUDIO_GSM			0x70
#define MEDIA_TYPE_AUDIO_GSM_EFR		0x71
#define MEDIA_TYPE_AUDIO_ILBC			0x72
#define MEDIA_TYPE_AUDIO_AMR			0x73
#define MEDIA_TYPE_AUDIO_AMR_WB			0x74
#define MEDIA_TYPE_AUDIO_SILK			0x75
#define MEDIA_TYPE_AUDIO_LPC			0x76
#define MEDIA_TYPE_AUDIO_LPC1016		0x77
#define MEDIA_TYPE_AUDIO_LPC1015		0x78
#define MEDIA_TYPE_AUDIO_L16			0x79
#define MEDIA_TYPE_AUDIO_SPEEX			0x7A
#define MEDIA_TYPE_AUDIO_L8				0x7B
#define MEDIA_TYPE_AUDIO_MPA			0x7C
#define MEDIA_TYPE_AUDIO_DVI4			0x7D
#define MEDIA_TYPE_AUDIO_VDVI			0x7E
#define MEDIA_TYPE_AUDIO_CN				0x7F
#define MEDIA_TYPE_AUDIO_RED			0x80
#define MEDIA_TYPE_AUDIO_QCELP			0x81
#define MEDIA_TYPE_AUDIO_EVRC0			0x82
#define MEDIA_TYPE_AUDIO_EVRCB0			0x83
#define MEDIA_TYPE_AUDIO_G729           0x84
#define MEDIA_TYPE_AUDIO_VIVOX          0x85

/*指导消息体选项类型*/
/*指导消息体选项类型-VOIP选项类型*/
#define OPT_SIP_SEND_RATE_INFO 			0x71  /*VOIP编码参数信息*/

/*==================================消息头部==============================================*/
typedef struct msg_head_s 
{
	uint16_t  			magic; 
	uint16_t 			m_type;	 	
	uint32_t 			c_len; 	       //消息体长度(不包括消息头部)
}msg_head_t;

/*==================================指导消息体==============================================*/
typedef struct msg_meta_s
{
	char				pid[8];			//ID，由数据获取节点生成
	uint64_t			proglen;		//流文件的总字节长度，为0表示长度未知
	uint32_t			capip;			// 获取节点的IP
	uint8_t				protocol;		// 承载协议类型
	uint8_t				mediatype;		// 可能的媒体类型
	uint8_t				data_flag;	    // 数据标志
	uint8_t				opt_num;    	// 选项个数 
}msg_meta_t;

/*已经在其他文件里面定义过
struct opt_unit_t
{
	uint32_t 			opt_len;        //包括opt_len、opt_type、opt_value
	uint8_t				opt_type; 
	char*				opt_value; 
};
*/

/*==================================流数据消息体==============================================*/
typedef struct av_data_s
{
	char				pid[8];			// 流文件的ID，由数据获取节点生成
	uint16_t			frag_seq;		// 音视频碎片:片段序号   VOIP:主被叫
	int64_t				offset:48;		// 本次数据在整个流文件中的偏移位置
	//char*				avs;			// 多媒体流数据
}av_data_t;

/*VOIP多媒体流数据格式*/
struct voip_header_t
{
	uint32_t   			time_stamp; 	 // 语音帧的时间戳, 网络序。
	uint32_t   			sequence;  
	//char*	 			data;       	 //VOIP的语音数据
}; 

/*RTSP/RTMP多媒体流数据格式*/
struct sdp_data_packet
{
	char 				sdp[8];			 // 恒为"RTSP_SDP"
	uint32_t 			sdp_len;	     // 为sdp的数据长度
	//char * 			data;			 // sdp数据内容
};

/*RTSP sdp数据内容格式*/
struct rtsp_data_packet
{
	uint32_t 			flag;			// RTSP数据头开头标志  0x46464646 "FFFF"
	uint8_t 			type;			// 数据类型，1 for RDT，2 for RTP
	uint8_t 			channel;		// 数据的通道号
	uint16_t 			len;			// 数据长度
	//char * 			data;			// 数据内容
};

/*RTMP sdp数据内容格式*/
struct rtmp_data_packet
{
	uint32_t  			flag;			// RTMP数据头开头标志  0x52544D50"RTMP"
	uint32_t  			av_type:8;	   	// 音视频类型，8 for Audio, 9 for Video
	uint32_t  			cs_id:24; 	   	// Chunk Stream ID
	uint32_t 			timestamp;	   	// 时间戳
	uint32_t 			len;			// 总数据长度(包括本头部字段长度，20字节)
	uint32_t  			stream_id;	   	// 流ID
	//char * 			data;		    // 数据内容
};

/*MMS多媒体流数据格式*/
struct mms_data_packet
{
	uint32_t 	 		frame_flag;		// 恒为0x46464646 "FFFF"
	uint32_t 	 	 	data_len;		// 为数据长度
	//char * 			data;			// 数据内容H包数据内容或者D包数据内容
};

/*==================================结果消息体==================================================*/
typedef struct msg_result_s
{
	char				pid[8];			// 本流文件的ID，由数据获取节点生成
	int					servicetype;	// 业务类型	
	uint32_t			cfgid;			// 命中的配置ID，值为0表示命中未知配置
	int8_t				level;			// 检测结果置信度， [0, 100]
	uint8_t				pad[7];			// 预留，必须填0值
	//char*  			logindex;		// 访问现场数据文件的存储路径URL，以"/0"结束
}msg_result_t;

#endif
