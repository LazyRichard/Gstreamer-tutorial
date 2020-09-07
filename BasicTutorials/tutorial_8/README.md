# Basic tutorial 8: Short-cutting the pipeline

## Goal

GStreamer을 통해 생성된 pipeline은 완전히 닫힐 필요는 없다. 데이터는 다양한 방법으로 언제든지 pipeline에 주입되거나 꺼낼 수 있다.

* GStreamer pipeline으로 어떻게 외부 데이터를 주입하는 지
* 일반적인 GStreamer pipeline으로부터 데이터를 어떻게 꺼내는지
* 어떻게 데이터에 엑세스하고 조작하는지.

## Introduction

어플리케이션은 여러 방법으로 GStreamer pipeline에 흐르는 데이터와 상호작용할 수 있다. 이번 예제에서는 이 목적을 위해서만 만들어진 element를 통해 가장 쉬운 방법으로 해보자.

어플리케이션으로부터 데이터를 GStreamer pipeline에 주입하기 위해서는 `appsrc`라는 element를 사용한다. 반대로 GStreamer 데이터를 어플리케이션으로 꺼내기 위해서는 `appsink` element를 사용한다. 이름에서 오는 혼동을 조금 줄이기 위해 GStreamer의 관점에서 보도록 하자. `appsrc`는 일반적인 source이다. 다만 데이터가 하늘에서 떨어질 뿐. (사실 어플리케이션이 주는 것이지만) `appsink`는 일반적인 sink이다. 다만 데이터가 GStreamer pipeline으로부터 사라질 뿐. (사실 어플리케이션이 가져가는 것이다.)

`appsrc`는 여러 모드로 동작할 수 있다. **pull**모드는 어플리케이션에게 데이터가 필요할 때마다 요청한다. **push** 모드는 어플리케이션이 데이터를 자신의 페이스대로 넣어준다. 더 나아가서 push 모드에서는 어플리케이션은 충분한 데이터가 이미 제공되었다고 생각되면 push 함수를 블럭할 수 있다. 또는 `enough-data`와 `need-data` 시그널을 이용해 제어를 할 수 있다. 이 예제에서는 후자의 접근 방법을 사용한다. 나머지는 `appsrc` 문서를 참고하시라.

## Buffers

GStreamer pipeline을 흐르는 데이터는 덩어리 형태로 **buffers*라고 부른다. 이 예제가 생산자와 소비자 형태의 데이터이므로 `GstBuffer`에 대해 알 필요가 있다.

Source Pad는 buffer를 생성하고 Sink Pad에서 소모된다. GStreamer은 이러한 buffer들을 element에서 다른 element로 전달한다.

buffer는 간단하게 표현하자면 데이터의 단위이다. 모든 buffer가 같은 크기나 동일한 길이의 시간으로 표현될 거라고 가정하지 마라. 또한 하나의 buffer가 element에 들어가면 하나의 buffer가 element로 부터 나올 것이라고 가정해서도 안 된다. element는 받은 buffer를 자유롭게 원하는 대로 할 수 있다. 실제 buffer 메모리는 GstMemory 객체를 통해 추상화되며, GstBuffer는 여러 개의 GstMemory 객체를 포함할 수 있다.

모든 buffer는 디코딩, 렌더 또는 표현되어야 할 순간에 대한 타임 스탬프와 길이를 갖는다. 타임 스탬프를 기록하는 것은 매우 복잡하고 섬세한 주제지만, 여기서는 단순화된 시각만으로도 충분하다.

예를 들면 `filesrc`(GStreamer의 element로 파일을 읽는다.)는 "ANY" caps로 buffer를 생산하는데, 타임 스템프에 대한 정보가 없다. 디멀티플렉싱 과정 이후에(Basic tutorial 3: Dynamic pipelines를 참고하라.) buffer는 "video/x-h264"와 같은 특정한 cap을 갖을 수 있다. 디코딩 이후에 각각의 buffer는 "video/x-raw-yuv"와 같은 raw cap과 하나의 영상 프레임과 이 프레임이 언제 보여져야 할지 기록된 매우 정밀한 타임 스탬프를 갖는다.

## This tutorial

이 예제는 Basic tutorial 7: Multithading and Pad Availability를 두 가지 방법으로 확장한다. 첫번째는 `audiotestsrc`가 `appsrc`로 변경되어 오디오 데이터를 생성한다. 두번째는 `tee` element에 새로운 브랜치가 추가되고 데이터는 오디오 sink와 웨이브 홈, 그리고 `appsink`로 복제된다. `appsink`는 어플리케이션으로 데이터를 업로드 하여 사용자에게 데이터를 받았다고 알린다. 물론 충분히 더 복잡한 작업도 수행할 수 있다.

![tutorial 8 pipeline](https://gstreamer.freedesktop.org/documentation/tutorials/basic/images/tutorials/basic-tutorial-8.png)

## A crude waveform genertor

![tutorial 8 result](/assets/tutorial_8.gif)

## Walkthrough

```c
/* Configure appsrc */
gst_audio_info_set_format(&info, GST_AUDIO_FORMAT_S16, SAMPLE_RATE, 1, NULL);
audio_caps = gst_audio_info_to_caps(&info);
g_object_set(data.app_source, "caps", audio_caps, "format", GST_FORMAT_TIME, NULL);
g_signal_connect(data.app_source, "need-data", G_CALLBACK(start_feed), &data);
g_signal_connect(data.app_source, "enough-data", G_CALLBACK(stop_feed), &data);
```

첫 번째 `appsrc`에 세팅해야 할 속성은 `caps`이다.  ... 이 속성은 반드시 `GstCaps` 객체여야 하며, `gst_caps_from_string()`로 쉽게 생성할 수 있다.

그 다음 `need-data`와 `enough-data` 시그널에 연결한다. 이 시그널들은 `appsrc`에 의해 내부 큐에 있는 데이터가 비었거나 거의 가득차면 실행된다. 이러한 시그너를 이용해 시그널 생성기 프로세스를 시작하거나 종료한다.

```c
/* Configure appsink */
g_object_set(data.app_sink, "emit-signals", TRUE, "caps", audio_caps, NULL);
g_signal_connect(data.app_sink, "new-sample", G_CALLBACK(new_sample), &data);
gst_caps_unref(audio_caps);
```

`appsink`의 설정에 따라 sink에 buffer가 들어올 때마다 실행되는 `new-sample` 시그널에 콜백을 연결한다. 이 시그널은 기본적으로 비활성화 되어 있기 때문에 `emit-signals` 속성을 이용해 활성화 시켜줘야 한다.

pipeline을 시작하고 메시지를 기다리고 마지막 정리하는 부분은 평소처럼 수행한다. 등록된 콜백 함수에 대해 알아보자.

```c
/* This signal callback triggers when appsrc needs data. Here, we add an idle handler
 * to the mainloop to start pushing data into the appsrc */
static void start_feed(GstElement *source, guint size, CustomData *data) {
  if (data->sourceid == 0) {
    g_print("\n");
    g_message("Start feeding");
    data->sourceid = g_idle_add((GSourceFunc)push_data, data);
  }
}
```

이 함수는 `appsrc`의 내부 큐가 거의 비었을 때 실행된다. 이 함수는 오직 GLib의 idle 함수를 `g_idle_add()`를 통해 등록하는 일이며, `appsrc`의 내부 큐가 거의 찰 때까지 데이터를 공급하도록 한다. GLib의 idle 함수는 GLib의 메인 루프가 아이들 상태일 때 호출되는 메서드로 높은 우선순위의 작업이 없을 때 동작한다. 물론 이는 GLib의 `GMainLoop`가 인스턴스화 되고 동작중이어야 한다.

이것은 `appsrc`가 허용하는 다양한 접근 방식 중 하나이다. ...

`g_idle_add()`가 반환하는 sourceid를 기록해 나중에 비활성화 할 수 있도록 한다.

```c
/* This callback triggers when appsrc has enough data and we can stop sending.
 * We remove the idle handler from the mainloop */
static void stop_feed(GstElement *source, CustomData *data) {
  if (data->sourceid != 0) {
    g_message("Stop feeding");
    g_source_remove(data->sourceid);
    data->sourceid = 0;
  }
}
```

이 함수는 `appsrc`의 내부 큐가 거의 가득차서 데이터를 그만 보내야 할 때 호출된다. 이 함수는 `g_source_remove()`를 이용해 간단히 idle 함수를 삭제한다.

```c
/* This method is called by the idle GSource in the mainloop, to feed CHUNK_SIZE bytes into appsrc.
 * The idle handler is added to the mainloop when appsrc requests us to start sending data (need-data signal)
 * and is removed when appsrc has enough data (enough-data signal).
 */
static gboolean push_data(CustomData *data) {
  GstBuffer *buffer;
  GstMapInfo map;
  gint16 *raw;
  gfloat freq;
  gint num_samples = CHUNK_SIZE / 2; // Because each sample is 16 bits
  GstFlowReturn ret = GST_FLOW_ERROR;

  /* Create a new empty buffer */
  buffer = gst_buffer_new_and_alloc(CHUNK_SIZE);

  /* Set its timestamp and duration */
  GST_BUFFER_TIMESTAMP(buffer) = gst_util_uint64_scale(data->num_samples, GST_SECOND, SAMPLE_RATE);
  GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(num_samples, GST_SECOND, SAMPLE_RATE);

  /* Generate some psychodelic waveforms */
  raw = (gint16 *)map.data;

  ...
```

이 함수는 `appsrc`에 데이터를 공급한다. GLib에 의해 호출되며 호출되는 시간과 빈도는 우리의 제어 밖이다. 다만 우리가 알 수 있는 것은 `appsrc`의 내부 큐가 가득차면 비활성화 될 것이라는 점이다.

첫 번째 테스크는 `gst_buffer_new_and_alloc()`를 이용해 주어진 크기로 새로운 buffer를 생성하는 것이다. (이 예제에서는 임의로 1024 바이트로 설정하였다.)

생성한 샘플의 개수를 `CustomData.num_samples` 변수에 기록하고 이를 이용해 `GstBuffer`의 `GST_BUFFER_TIMESTAMP`로 buffer에 타임 스탬프를 기록한다.

`gst_util_uint64_scale()`은 유틸리티 함수로 오버플로우의 걱정 없이 숫자의 스케일을 조정할 수 있다.

```c
/* Push the buffer into appsrc */
g_signal_emit_by_name(data->app_source, "push-buffer", buffer, &ret);

/* Free the buffer now that we are done with it */
gst_buffer_unref(buffer);
```

버퍼가 준비되면 `push-buffer` 액션 시그널을 통해 `appsrc`에 전달하고(Playback tutorial 1: Playbin usage의 정보 박스를 확인하라.), 더 이상 버퍼가 필요하지 않으므로 `gst_buffer_unref()`를 이용해 해제한다.

```c
/* The appsink has received abuffer */
static GstFlowReturn new_sample(GstElement *sink, CustomData *data) {
  GstSample *sample;
  GstFlowReturn ret = GST_FLOW_ERROR;

  /* Retrieve the buffer */
  g_signal_emit_by_name(sink, "pull-sample", &sample);
  if (sample) {
    /* The only thing we do in this example is print a * to indicate a received buffer */
    g_print("*");
    gst_sample_unref(sample);

    ret = GST_FLOW_OK;
  }

  return ret;
}
```

마지막으로 이 함수는 `appsink`가 buffer를 받았을 때 실행된다. `pull-sample` 액션 시그널을 이용해 buffer를 가져오고 알 수 있도록 화면에 표시한다. `GstBuffer` 안에 있는 데이터 포인터를 `GST_BUFFER_DATA` 메크로와 `GST_BUFFER_SIZE` 메크로로 데이터와 데이터의 크기를 가져올 수 있다. 이 버퍼는 지나온 경로상에 있는 어떤 element에서 변경할 기회가 있었기 때문에 `push-data` 함수에서 생산한 buffer와 일치하지 않을 수도 있다는 점을 기억해야 한다(물론 이 예제에서는 `appsrc`와 `appsink` 사이의 경로에는 `tee` element만 있었기 때문에 buffer의 내용은 변경되지 않는다.).

마지막으로 `gst_buffer_unref()`를 이용해 buffer를 해제해주면 예제가 끝이 난다.

# Conclusion

* `appsrc` element를 이용해 pipeline에 데이터를 주입하는 방법
* `appsink` element를 이용해 pipeline으로 부터 데이터를 추출하는 방법
* `GstBuffer`를 접근함으로써 데이터를 수정하는 방법

# References

* Gstreamer official tutorials - [Basic tutorial 8: Short-cutting the pipeline
Goal](https://gstreamer.freedesktop.org/documentation/tutorials/basic/short-cutting-the-pipeline.html?gi-language=c)
