#include "Sampler.h"

void Sampler::setup()
{
	if (audio.getDeviceCount() < 1)
	{
		cerr << "No audio devices found!" << endl;
		exit(0);
	}
	cout << "Available devices are " << endl;
	vector<unsigned int> ids = audio.getDeviceIds();

	for (unsigned int id : ids)
	{
		RtAudio::DeviceInfo info = audio.getDeviceInfo(id);
		cout << info.name << endl;
	}
	// for (unsigned i = 0; i < ids.size(); i++)
	// {
	// 	RtAudio::DeviceInfo info = audio.getDeviceInfo(i);
	// 	cout << info.name << endl;
	// }
	cout << endl;

	iParams.deviceId = audio.getDefaultInputDevice();
	iParams.nChannels = IN_CHANNELS;

	oParams.deviceId = audio.getDefaultOutputDevice();
	oParams.nChannels = OUT_CHANNELS;

	cout << "Using input device " << audio.getDeviceInfo(iParams.deviceId).name << endl;
	cout << "Using output device " << audio.getDeviceInfo(oParams.deviceId).name << endl;
	cout << endl;

	if (!(audioData.inBuffer = (double *)malloc(BUFFER_FRAMES * sizeof(double))))
	{
		cerr << "Couldn't allocate data" << endl;
	}
}

void Sampler::openStream()
{
	unsigned int nBufferFrames = BUFFER_FRAMES;
	unsigned int sampleRate = SAMPLE_RATE;
	if (audio.openStream(&oParams, &iParams, RTAUDIO_FLOAT64, sampleRate,
						 &nBufferFrames, &recAndPlay, static_cast<void *>(&audioData)))
	{
		std::cout << '\n'
				  << audio.getErrorText() << '\n'
				  << std::endl;
		exit(0); // problem with device settings
	}
	if (audio.startStream())
	{
		std::cout << '\n'
				  << audio.getErrorText() << '\n'
				  << std::endl;
		if (audio.isStreamOpen())
			audio.closeStream();
	}

	// try {
	// 	audio.openStream( &oParams, &iParams, RTAUDIO_FLOAT64, sampleRate,
	// 			&nBufferFrames, &recAndPlay, static_cast<void*>(&audioData));
	// 	audio.startStream();
	// } catch (RtAudioErrorType &error) {
	// 	error.printMessage();
	// 	exit(0);
	// }

	cout << "Audio stream opened at device " << endl;
}

void Sampler::closeStream()
{
	if (audio.isStreamRunning())
		audio.stopStream();
	// try
	// {
	// 	audio.stopStream();
	// }
	// catch (RtAudioErrorType &e)
	// {
	// 	e.printMessage();
	// }

	if (audio.isStreamOpen())
		audio.closeStream();

	cout << "Audio stream closed" << endl;
}

//----------------------------------------------------------------
void Sampler::newSample(const char name, const float lengthInSec)
{
	if (getSampleIndex(name) != -1)
	{
		cerr << "Samplename " << (unsigned)name << " already exists!" << endl;
		return;
	}

	SamplerSample sample;

	sample.name = name;
	sample.bufferSize = SAMPLE_RATE * 4; //<-------- Set static at 4 seconds
	sample.sampleRate = SAMPLE_RATE;
	sample.bufferFrames = BUFFER_FRAMES;
	sample.inChannels = IN_CHANNELS;
	sample.outChannels = OUT_CHANNELS;
	sample.threshold = THRESHOLD;

	if (!(sample.buffer = (double *)malloc(sample.bufferSize * sizeof(double) * IN_CHANNELS)))
	{
		cerr << "Failed to allocate memory" << endl;
		exit(0);
	}

	audioData.samples.push_back(sample);
	cout << "Added new sample " << (unsigned)sample.name << " of length " << lengthInSec << endl;
}

//----------------------------------------------------------------
void Sampler::record(const char name)
{
	int i = getSampleIndex(name);
	audioData.samples[i].index = i;

	if (audioData.samples[i].state == PRIMED)
	{
		return;
	}

	if (i == -1)
	{
		cerr << "No sample found with name " << name << endl;
		exit(0);
	}
	else
	{
		audioData.samples[i].state = PRIMED;
		audioData.samples[i].rechead = 0;

		cout << "Primed track " << name << endl;
	}
}

void Sampler::play(const char name, const float positionInSec)
{
	int i = getSampleIndex(name);

	if (i == -1)
	{
		cerr << "No sample found with name " << name << endl;
		exit(0);
	}
	else
	{
		float positionInFrames = positionInSec * SAMPLE_RATE;

		audioData.samples[i].playhead = positionInFrames;
		audioData.samples[i].positionInFrames = positionInFrames;
		audioData.samples[i].state = PLAY;
		audioData.samples[i].isFade = true;
		audioData.samples[i].isFadeDown = true;

		//		cout << "Playing track " << name  << " of length " << audioData.samples[i].bufferSize << " from position " << positionInFrames << endl;
	}
}

//----------------------------------------------------------------
bool Sampler::isRecorded(const char &name)
{
	int i = getSampleIndex(name);
	if (i == -1)
		cerr << "No sample found with name " << name << endl;
	else
		return audioData.samples.at(i).isRecorded;

	return false;
}

state_t Sampler::getSampleState(const char &name)
{
	int i = getSampleIndex(name);
	if (i == -1)
		cerr << "No sample found with name " << name << endl;
	else
		return audioData.samples.at(i).state;

	return STOP;
}

float Sampler::getSamplePlayhead(const char &name)
{
	int i = getSampleIndex(name);
	unsigned playheadInFrames = 0;
	if (i == -1)
		cerr << "No sample found with name " << name << endl;
	else
		playheadInFrames = audioData.samples.at(i).playhead;

	float playheadInSec = (float)playheadInFrames / SAMPLE_RATE;
	return playheadInSec;
}

double Sampler::getAmplitude()
{

	double amp = 0;
	for (int i = 0; i < BUFFER_FRAMES; i++)
	{
		if (audioData.inBuffer[i] > amp)
			amp = audioData.inBuffer[i];
	}

	return amp;
}

// Private
//----------------------------------------------------------------
int Sampler::getSampleIndex(const char &name)
{
	for (int i = 0; i < audioData.samples.size(); i++)
	{
		if (audioData.samples[i].name == name)
			return i;
	}

	return -1;
}

// Outside (But want to move in?)
//----------------------------------------------------------------
int recAndPlay(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
			   double streamTime, RtAudioStreamStatus status, void *userData)
{

	audioData_t *audioData = static_cast<audioData_t *>(userData);
	vector<SamplerSample> *samples = &audioData->samples;

	double *inBuffer = static_cast<double *>(inputBuffer);
	double *outBuffer = static_cast<double *>(outputBuffer);
	audioData->inBuffer = inBuffer;

	memset(outputBuffer, 0, nBufferFrames * OUT_CHANNELS * sizeof(double));

	if (status)
		cerr << "Stream overflow detected!" << endl;

	for (auto &sample : *samples)
	{
		if (sample.state == REC)
		{
			sample.record(inBuffer);
		}
		else if (sample.state == PLAY)
		{
			sample.play(outBuffer);
		}
		else if (sample.state == STOP)
		{
		}
		else if (sample.state == IDLE)
		{
		}
		else if (sample.state == PRIMED)
		{
			sample.record(inBuffer);
		}
		else
		{
			cerr << "Couldn't get state from sample" << endl;
		}
	}
	return 0;
}
