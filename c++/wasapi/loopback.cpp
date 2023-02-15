#include <Windows.h>
#include <Mmdeviceapi.h>
#include <Audioclient.h>
#include <iostream>

using namespace std;

int main() {
    HRESULT hr;
    IMMDeviceEnumerator* pEnumerator = NULL;
    IMMDevice* pDevice = NULL;
    IAudioClient* pAudioClient = NULL;
    IAudioCaptureClient* pCaptureClient = NULL;
    WAVEFORMATEX* pWaveFormat = NULL;
    REFERENCE_TIME hnsRequestedDuration = 10000000; // 1초에 해당하는 REFERENCE_TIME

    // 디바이스 열거자 생성
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        cerr << "Failed to create device enumerator" << endl;
        goto exit;
    }

    // 기본 레코딩 디바이스 가져오기
    hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pDevice);
    if (FAILED(hr)) {
        cerr << "Failed to get default capture device" << endl;
        goto exit;
    }

    // IAudioClient 인터페이스 얻기
    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
    if (FAILED(hr)) {
        cerr << "Failed to get audio client" << endl;
        goto exit;
    }

    // WaveFormat 정보 얻기
    hr = pAudioClient->GetMixFormat(&pWaveFormat);
    if (FAILED(hr)) {
        cerr << "Failed to get mix format" << endl;
        goto exit;
    }

    // IAudioClient 초기화
    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, hnsRequestedDuration, 0, pWaveFormat, NULL);
    if (FAILED(hr)) {
        cerr << "Failed to initialize audio client" << endl;
        goto exit;
    }

    // IAudioCaptureClient 인터페이스 얻기
    hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);
    if (FAILED(hr)) {
        cerr << "Failed to get capture client" << endl;
        goto exit;
    }

    // 레코딩 시작
    hr = pAudioClient->Start();
    if (FAILED(hr)) {
        cerr << "Failed to start recording" << endl;
        goto exit;
    }

    // 루프백 데이터 출력
    DWORD dwNumFrames = 0;
    BYTE* pData = NULL;
    DWORD dwFlags = 0;
    UINT32 numFramesAvailable = 0;

    // 1초간 녹음하고 루프백 데이터 출력
    Sleep(1000);
    hr = pCaptureClient->GetNextPacketSize(&numFramesAvailable);
    if (FAILED(hr)) {
        cerr << "Failed to get next packet size" << endl;
        goto exit;
    }
    while (numFramesAvailable > 0) {
        // 패킷 데이터 가져오기
        hr = pCaptureClient->GetBuffer(&pData, &dwNumFrames, &dwFlags, NULL, NULL);
        if (FAILED(hr)) {
            cerr << "Failed to get buffer" << endl;
            goto exit;
        }

        // 루프백 데이터 출력
        fwrite(pData, pWaveFormat->nChannels * sizeof(float), dwNumFrames, stdout);

        // 버퍼 반환
        hr = pCaptureClient->ReleaseBuffer(dwNumFrames);
        if (FAILED(hr)) {
            cerr << "Failed to release buffer" << endl;
            goto exit;
        }

        // 다음 패킷 사이즈 가져오기
        hr = pCaptureClient->GetNextPacketSize(&numFramesAvailable);
        if (FAILED(hr)) {
            cerr << "Failed to get next packet size" << endl;
            goto exit;
        }
    }

exit:
    // 정리
    if (pCaptureClient != NULL) {
        pCaptureClient->Release();
    }
    if (pAudioClient != NULL) {
        pAudioClient->Stop();
        pAudioClient->Release();
    }
    if (pDevice != NULL) {
        pDevice->Release();
    }
    if (pEnumerator != NULL) {
        pEnumerator->Release();
    }
    if (pWaveFormat != NULL) {
        CoTaskMemFree(pWaveFormat);
    }

    return 0;
}

