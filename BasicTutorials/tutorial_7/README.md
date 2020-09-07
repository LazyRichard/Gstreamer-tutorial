# Basic tutorial 7: Multithreading and Pad Availability

## Goal

GStreamer는 자동으로 다중 스레드를 관리한다. 하지만 몇몇 상황에 따라 스레드를 수동으로 디커플 해야 할 수도 있다.

* 파이프리인의 몇몇 pad를 어떻게 스레드로 만들어서 실행하는지
* Pad 가용성이란 무엇인지
* 어떻게 스트림을 복제하는지

## Introduction

## Multithreading

GStreamer는 멀티 스레드 프레임워크이다. 이 말은 내부적으로 스레드를 필요에 따라 만들고 지운다. 예를 들면 어플리케이션 스레드로 부터 스트리밍을 분리하는 것이 있다. 더 아나가서 플러그인은 스스로 자유롭게 스레드를 만들어 자신만의 처리를 수행할 수 있다. 예를 들면 비디오 디코더는 4개의 스레드를 만들어 CPU 코어가 4개일 때 최대 장점을 가져간다.

...

## The example pipeline

![example pipeline](https://gstreamer.freedesktop.org/documentation/tutorials/basic/images/tutorials/basic-tutorial-7.png)

소스는 합성된 오디오 시그널로 `tee` 객체를 통해 복제한다(이는 sink pad로 부터 들어온 모든 데이터를 source pad로 전달한다.). 하나의 브랜치는 오디오 카드로 보내고, 나머지는 웨이브 폼의 형태로 화면으로 출력한다.

사진에서 볼 수 있듯이 큐는 새로운 스레드를 만든다. 따라서 pipeline에는 총 3개의 스레드가 동작한다. 하나 이상의 sink를 갖는 pipeline은 일반적으로 멀티스레딩이 필요한데, sink가 서로 동기를 맞추려면 다른 모든 sink가 준비될 때까지 블럭할 필요가 있다. 만약 오직 하나의 스레드만 존재한다면 첫 번째 sink가 블럭되면 준비 작업을 진행할 수 없기 때문이다.

## Request pads

Basic tutorial 3: Dynamic pipelines에서 볼 수 있듯이 몇몇 element(`uridecodebin`)는 시작 시에는 아무런 pad도 없다가 데이터가 흐르기 시작하고 element가 미디어에 대해 알기 시작한 다음 부터 나타난다. 이를 **Sometimes Pad**라고 부른다. 그와 반대로 항상 사용가능한 일반적인 pad는 **Always Pads**라고 부른다.

3 번째 종류로 **Request Pad**가 있는데 이는 요청에 따라 생성된다. 대표적인 예로 `tee` element가 있는데, 하나의 sink pad를 갖고 초기에는 source pad를 갖지 않는다. 그러다 `tee` 객체에 요청이 들어오면 필요에 따라 pad를 생성한다. 이러한 결과로 입력 스트림은 임의의 갯수로 복제될 수 있다. 이러한 방식의 단점은 Request Pad를 통한 element간 연결은 Always Pad 처럼 자동으로 수행되지 않는다. 이 방법은 예제를 통해 볼 수 있을 것이다.

그리고 이 예제에서는 설명하지 않겠지만 `GST_STATE_PLAYING`또는 `GST_STATE_PAUSED` 상태에서 pad를 요청 또는 해제 작업은 Pad 블럭킹과 같은 몇몇 추가적인 주의를 기울여야 한다. pad를 요청하거나 해제하는 작업은 `GST_STATE_NULL` 또는 `GST_STATE_READY` 상태에서 하는 것이 안전하다.

## Simple multithreaded example

![Simple multithreaded example result](/assets/tutorial_7.gif)

## Walkthrough

```c
/* Create the elements */
audio_source = gst_element_factory_make("audiotestsrc", "audio_source");
tee = gst_element_factory_make("tee", "tee");
audio_queue = gst_element_factory_make("queue", "audio_queue");
audio_convert = gst_element_factory_make("audioconvert", "audio_convert");
audio_resample = gst_element_factory_make("audioresample", "audio_resample");
audio_sink = gst_element_factory_make("autoaudiosink", "audio_sink");
video_queue = gst_element_factory_make("queue", "video_queue");
visual = gst_element_factory_make("wavescope", "visual");
video_convert = gst_element_factory_make("videoconvert", "csp");
video_sink = gst_element_factory_make("autovideosink", "video_sink");
```

모든 element는 위에 코드 조각 처럼 인스턴스화 한다.

`audiotestsrc`는 톤을 합성해준다. `wavescope`는 오디오 시그널을 받아서 오실로 스코프처럼 웨이브 폼을 생성해준다. 이미 `autoaudiosink`와 `autovideosink`는 했으니 넘어가겠다.

변환 element는 (`audioconvert`, `audioresample`과 `videoconvert`)는 파이프라인이 연결될 수 있음을 보장하기 위해 필요하다. 실제로 오디오 및 비디오 sink는 하드웨어에 따라 다르며 디자인을 하는 시점에서는 `audiotestsrc`와 `wavescope`에서 만든 Caps와 일치하는지 알 수 없다. 만약 Caps가 일치한다면 이 element는 패스-스루 형태로 동작해 시그널을 손대지 않아 성능에 거의 영향을 주지 않는다.

```c
/* Configure elements */
g_object_set(audio_source, "freq", 215.0f, NULL);
g_object_set(visual, "shader", 0, "style", 1, NULL);
```

시연을 조금 더 수월하게 하기 위해 몇 가지 자그마한 조정을 거쳤다. `audiotestsrc`의 "freq" 프로퍼티로 출력 웨이브의 주파수를 제어할 수 있다(215Hz는 파형이 화면상에 거의 정지한 것처럼 보인다.). 그리고 `wavescope`의 "style"과 "shader" 프로퍼티는 파형을 이어지도록 한다. Basic tutorial 10: GStreamer tools에 설명된 `gst-inspect-1.0` 도구를 사용하면 element의 모든 프로퍼티에 대해 알 수 있다.

```c
/* Link all elements that can be automatically linked because they have "Always" pads */
gst_bin_add_many(GST_BIN(pipeline), audio_source, tee, audio_queue, audio_convert, audio_resample, audio_sink,
                  video_queue, visual, video_convert, video_sink, NULL);
if (gst_element_link_many(audio_source, tee, NULL) != TRUE ||
    gst_element_link_many(audio_queue, audio_convert, audio_resample, audio_sink, NULL) != TRUE ||
    gst_element_link_many(video_queue, visual, video_convert, video_sink, NULL) != TRUE) {
  g_error("Elements could not be linked.\n");
  gst_object_unref(pipeline);
  return -1;
}
```

*`gst_element_link_many()`는 사실 element의 Request Pad와 연결할 수 있다. 내부적으로 Pad를 요청하기 때문에 element와 연결된 pad가 Always Pad인지 Request Pad인지 신경쓰지 않아도 됀다. 조금 이상한 점을 느꼈겠지만 사실 이 점은 매우 불편하다. 왜냐하면 Request Pad를 사용한 다음 해제하는 과정이 필요하기 때문이다. 만약 `gst_element_link_many()`를 통해 자동으로 Request Pad에 연결되었다면 해재하는 과정을 잊어버리기 쉽다. 이러한 문제에 고통받고 싶지 않다면 항상 Request Pad는 다음 코드 조각처럼 수동으로 요청하라.*

```c
/* Manually link the Tee, which has "Request" pad */
tee_audio_pad = gst_element_get_request_pad(tee, "src_%u");
g_print("Obtained request pad %s for audio branch.\n", gst_pad_get_name(tee_audio_pad));
queue_audio_pad = gst_element_get_static_pad(audio_queue, "sink");
tee_video_pad = gst_element_get_request_pad(tee, "src_%u");
tee_video_pad = gst_element_get_request_pad(tee, "src_%u");
g_print("Obtained request pad %s for video branch.\n", gst_pad_get_name(tee_video_pad));

queue_video_pad = gst_element_get_static_pad(video_queue, "sink");
if (gst_pad_link(tee_audio_pad, queue_audio_pad) != GST_PAD_LINK_OK ||
    gst_pad_link(tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK) {
  g_error("Tee could not be linked.\n");
  gst_object_unref(pipeline);

  return -1;
}

gst_object_unref(queue_audio_pad);
gst_object_unref(queue_video_pad);
```

Request Pad에 연결하기 위해서는 element에 "요청"하는 과정이 필요하다. element는 다른 종류의 Request Pad를 만들어 줄 수도 있다. 따라서 요청을 할 때, 원하는 Pad 템플릿 이름을 꼭 전달해주어야 한다. `tee` element의 문서에는 `sink`와 `src_%u` 이렇게 두 개의 pad 템플릿을 확인할 수 있다. 오디오와 비디오 브랜치에 스트림을 복제하기 위해 `tee` element로 부터 `gst_element_get_request_pad()`를 이용해 2개의 pad를 요청한다.

요청한 Request Pad를 다운스트림의 element와 연결하기 위해 Pad를 가져와야 한다. 이 pad들은 일반적인 Always Pad이므로 `gst_element_get_static_pad()`를 통해 가져올 수 있다.

마지막으로 `gst_pad_link()`를 이용해 서로 연결한다. 이 함수는 `gst_element_link()`와 `gst_element_link_many()`에서 내부적으로 사용한다.

우리가 가져온 sink pad는 `gst_object_unref()`를 이용해 해제할 필요가 있다. Request Pad는 프로그램이 끝나 더 이상 필요 없을 때 해제 될 것이다.

그런 다음 pipeline을 평소처럼 재생 시키고 에러나 EOS 메시지가 들어올 때까지 기다린다. 마지막 할일은 나머지 자원들을 모두 해제하는 것이다.

```c
/* Release the request pads from the Tee, and unref them */
gst_element_release_request_pad(tee, tee_audio_pad);
gst_element_release_request_pad(tee, tee_video_pad);
gst_object_unref(tee_audio_pad);
gst_object_unref(tee_video_pad);
```

`gst_element_release_request_pad()`는 `tee` element로 부터 요청한 pad를 해제한다. 하지만 그래도 `gst_object_unref()`를 이용해 메모리 할당을 해제할 필요가 있다.

# Conclusion

* `element`요소를 이용해 pipeline의 일부분을 다른 스레드에서 동작하게 하는 법
* Request Pad가 무엇인지, 또 element와 어떻게 연결하는지 (`gst_element_get_request_pad()`, `gst_pad_link()`, `gst_element_release_request_pad()`)
* `tee` element를 이용해 하나의 스트림을 복제하는 방법

# References

* Gstreamer official tutorials - [Basic tutorial 7: Multithreading and Pad Availability](https://gstreamer.freedesktop.org/documentation/tutorials/basic/multithreading-and-pad-availability.html?gi-language=c)
