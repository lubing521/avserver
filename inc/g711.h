struct g711_encoder {
	struct stream *output;
	struct stream_destination *input;
	int channels;
	int frame_size;
	int block_align;
	int nUseBytesPerBlk;
	int samprate;
	struct soft_queue *inq;
	struct soft_queue *outq;
	pthread_t thread;
};

int g711_encode_frame(struct g711_encoder *avctx, unsigned char *frame, void *data, int length);
//int g711_encode_init();
#define BlkSize 256
//#define BlkSize 512