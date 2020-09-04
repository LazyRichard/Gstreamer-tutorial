# Basic tutorial 3: Dynamic pipelines

## Goal

* element 끼리 링크를 수립하는 과정을 좀더 정교하게 할 수 있다.
* 관심있는 이벤트에 대해 알림을 받아 제때 반응 할 수 있다.
* element가 같을 수 있는 다양한 상태들에 대해 알 수 있다.

## Introduction

만약 컨테이너가 하나의 영상과 두 개의 오디오 트랙과 같이 여러 스트림을 포함하고 있다면 demuxer는 이를 스트림별로 분리해서 각각 다른 출력 포트에 내보낸다.

element의 포트들은 pads(`GstPad`)를 통해 서로 통신한다. pad는 데이터가 element로 들어오는 sink pad와 데이터가 빠져나가는 source pad가 존재한다. source element는 source pad만 있고, sink element에는 sink pad만 있다. filter element에는 source pad와 sink pad 모두 존재한다.

![source element](https://gstreamer.freedesktop.org/documentation/tutorials/basic/images/src-element.png)

![filter element](https://gstreamer.freedesktop.org/documentation/tutorials/basic/images/filter-element.png)

![sink element](https://gstreamer.freedesktop.org/documentation/tutorials/basic/images/sink-element.png)

demuxer는 하나의 다중화된 데이터가 도착하는 sink pad와 컨테이너에서 찾은 각 스트림에 대해 각각 source pad를 갖는다.

![demuxer with two source pads.](https://gstreamer.freedesktop.org/documentation/tutorials/basic/images/filter-element-multi.png)

아래는 단순화된 pipeline으로 demuxer를 포함하고 두 개의 브랜치를 통해 하나는 오디오 하나는 비디오를 출력한다. 물론 예제에서 구현한 pipeline은 아니다.

![Example pipeline with two branches](https://gstreamer.freedesktop.org/documentation/tutorials/basic/images/simple-player.png)

demuxer를 사용하는데 가장 큰 문제는 데이터가 들어와서 컨테이너 안을 들여다보기 전까지 어떠한 정보도 생산할 수 없다는 것이다. 다시 말하면 demuxer가 데이터를 받아서 스트림을 분석하기 전까지 source pad가 생성되지 않는다는 것이다. 이 상태의 demuxer는 다른 element와 연결할 수 있는 source pad 없이 시작되므로 pipeline은 이 지점에서 종료된다.

이를 해결하기 위해서 pipeline을 source에서 demuxer까지 생성하고 상태를 play로 만든다. demuxer가 몇 개의 스트림이 있는지 충분히 확인할 정도의 데이터를 받으면 source pad를 생성할 것이다. 이 시점이 pipeline 생성을 종료하고 새로 추가된 demuxer pads와 연결할 기회이다.

이 예제에서는 단순화를 위해 오디오 pad만 연결하고 video는 무시한다.

## Dynamic Hello World

* No result due to AUDIO ONLY

## Walkthrough

```c
/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
  GstElement *pipeline;
  GstElement *source;
  GstElement *convert;
  GstElement *sink;
} CustomData;
```

이번 튜토리얼에서는 콜백이 관여하기 때문에 필요한 모든 정보들을 손쉽게 관리할 수 있도록 묶어서 처리한다.

```c
/* Create the elements */
data.source = gst_element_factory_make ("uridecodebin", "source");
data.convert = gst_element_factory_make ("audioconvert", "convert");
data.resample = gst_element_factory_make ("audioresample", "resample");
data.sink = gst_element_factory_make ("autoaudiosink", "sink");
```

`uridecodebin`은 내부적으로 sources, demuxers, decoders를 모두 만들어 입력된 URI를 raw 오디오 또는 비디오 스트림으로 출력한다.
얘는 demuxers를 포함하고 있기 때문에 초기에는 source pads를 사용할 수 없고, 동작 중간에 연결해야 한다.

`audioconvert`는 여러 오디오 포멧들을 변환하는데 유용한 element로 이 예제가 어떤 플랫폼이든 동작할 수 있도록 만든다.
audio decoder를 통해 나온 결과가 audio sink element가 예상한 것과 다를 수 있기 때문이다.

`audioresample`은 audio decoder를 통해 생성된 오디오 샘플 레이트가 audio sink에서 지원되지 않을 수도 있기 때문에 서로 다른 샘플 레이트간 변환하는데 유용하다.

`autoaudiosink`는 `audovideosink`처럼 오디오를 사운드 카드에 출력하는 역할을 한다.

```c
/* Build the pipeline. Note that we are NOT linking the source at this
 * point. We will do it later. */
gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.convert, data.resample, data.sink, NULL);
if (!gst_element_link_many(data.convert, data.resample, data.sink, NULL))
{
  g_error("elements could not be linked.");
  gst_object_unref(data.pipeline);

  return -1;
}
```

파이프라인에 source, convert, resample, sink를 모두 추가했지만 연결은 convert, resample, sink 간에만 수립한 것을 확인할 수 있다.
source와 연결하지 않은 이유는 아직 source element는 source pads를 가지고 있지 않기 때문이다.

```c
const gchar *uri = "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";

...

/* Set the URI to play */
g_object_set(data.source, "uri", uri, NULL);
```

이전 튜토리얼에서 했던 것처럼 재생할 영상에 대한 uri를 지정해준다.

### Signals

`GSignals`는 GStreamer에서 가장 중요한 부분이다. 뭔가 관심 있는 일이 발생한 것을 callback의 형태로 알림을 받을 수 있다.
시그널은 이름으로 구분되며 각각의 `GObject`들은 자신만의 시그널을 갖고 있다.

```c
/* Connect to the pad-added signal */
g_signal_connect (data.source, "pad-added", G_CALLBACK (pad_added_handler), &data);
```

`uridecodebin`의 `pad-added`라는 시그널에 `pad_added_handler`를 부착했다. 이렇게 하기 위해 `g_signal_connect()` 함수에
callback 함수(`pad_added_handler`)와 데이터 포인터(`CustomData`)를 넣어주었다. GStreamer는 데이터 포인터에 대해서 아무 것도 하지 않으므로
그냥 callback에 넘겨줘서 정보를 공유하면 된다. 이 튜토리얼에서는 `CustomData`에 이러한 정보들을 넣어두었다.

`GstElement`가 보내는 시그널에 대한 정보는 `gst-inspect-1.0`을 통해 확인할 수 있다.
자세한 정보는 [Basic tutorial 10: GStreamer tools](https://gstreamer.freedesktop.org/documentation/tutorials/basic/gstreamer-tools.html) 여기를 참고하라.

### The callback

source element가 데이터를 생산하기 위해 충분한 정보를 모았을 때, source pads를 생성하고 `pad-added` 시그널을 트리거 한다.
이 시점에 callback이(여기서는 `pad_added_handler`) 실행된다.

```c
static void pad_added_handler (GstElement *src, GstPad *new_pad, CustomData *data);
```

`src`는 시그널을 트리거 하는 `GstElement`이다(여기서는 `uridecodebin`). 시그널 핸들러의 첫 번째 파라미터는 항상 시그널을 트리거 하는 객체다.

`new_pad`는 `src` element에 추가되는 `GstPad`다. 대부분의 경우 이는 링크로 연결해야 하는 대상 pad다.

`data`는 시그널에 부착할 때, 제공되는 데이터이다. 여기서는 `CustomData`를 포인터의 형태로 넘겨주었다.

```c
GstPad *sink_pad = gst_element_get_static_pad (data->convert, "sink");
```

`CustomData`로 부터 conterter element의 sink pad를 `gst_element_get_statid_pad()`함수로 가져온다.
이 pad는 새롭게 생성되는 `new_pad`와 연결할 pad이다. 이전 튜토리얼에서는 element 끼리 연결 하여 GStreamer가 적당한 pad에 연결 할 수 있도록 했는데,
이번에는 pad까리 직접 연결할 예정이다.

```c
/* If our converter is already linked, we have nothing to do here */
if (gst_pad_is_linked (sink_pad)) {
  g_message ("We are already linked. Ignoring.\n");

  goto exit;
}
```

`uridecodebin` 은 여러 개의 pad를 생성할 수 있고, 이 때마다 callback은 호출된다. 이 코드 조각은 새로운 pad가 이미 연결된 pad에 연결되는 것을 방지해준다.

```c
/* Check the new pad's type */
new_pad_caps = gst_pad_get_current_caps (new_pad, NULL);
new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
new_pad_type = gst_structure_get_name (new_pad_struct);
if (!g_str_has_prefix (new_pad_type, "audio/x-raw")) {
  g_message ("It has type '%s' which is not raw audio. Ignoring.\n", new_pad_type);

  goto exit;
}
```

이번 튜토리얼에서는 오디오만 출력할 예정이므로 출력할 타입에 대한 검사를 한다.

`gst_pad_get_current_caps()`는 `GstCaps` 구조체로 감싸져 있는 pad의 기능(여기서는 현재 출력되는 데이터의 종류)를 확인한다.
pad가 지원할 수 있는 caps는 `gst_pad_query_caps()`를 통해 확인할 수 있다. pad는 많은 기능을 제공할 수 있고,
`GstCaps`는 다양한 `GstStructure`를 포함할 수 있다. pad의 현재 caps는 항상 단일 `GstStructure`를 가지며,
단일 미디어 형식을 나타내거나 사용하능한 caps가 없는 경우 NULL을 반환한다.

이 튜토리얼에서는 오직 오디오 기능에만 관심이 있기 때문에 `gst_caps_get_structure()`를 통해 첫 번째 `GstStructure`만 가져온다.

마지막으로 `gst_structure_get_name()`를 통해 미디어 타입 이름을 가져온다.
만약 타입이 `audio/x-raw`가 아니라면 이는 디코드 된 오디오 pad가 아니므로 무시한다.

```c
/* Attempt the link */
ret = gst_pad_link (new_pad, sink_pad);
if (GST_PAD_LINK_FAILED (ret)) {
  g_message ("Type is '%s' but link failed.\n", new_pad_type);
} else {
  g_message ("Link succeeded (type '%s').\n", new_pad_type);
}
```

`gst_pad_link()`는 두 개의 pads를 연결한다. `gst_element_link()`의 경우처럼 링크는 source에서 sink로 나타나야 하며,
두 pad가 있는 element는 같은 bin(또는 pipeline)에 속해있어야 한다.

### GStreamer states

State|Description
---|---
`GST_STATE_NULL`|NULL 상태 또는 element의 초기 상태
`GST_STATE_READY`|element가 준비되어서 PAUSED로 가는 상태.
`GST_STATE_PAUSED`|element가 PAUSED 된 상태로 데이터를 받아들이고 가공할 준비가 된 상태이다. 하지만 sink element는 오직 하나의 버퍼만 받고 블럭한다.
`GST_STATE_PLAYING`|element가 PLAYING 상태로 클럭이 돌고 데이터가 흐른다.

상태는 한 칸씩만 이동할 수 있다. 즉 `GST_STATE_NULL`에서 `GST_STATE_PLAYING`으로 건너뛸 수 없고
중간 상태인 `GST_STATE_READY`와 `GST_STATE_PAUSED`를 거쳐야 한다.
만약 pipeline을 `GST_STATE_PLAYING`으로 설정한다면 GStreamer는 중간 상태를 거치도록 자동으로 설정할 것이다.

```c
case GST_MESSAGE_STATE_CHANGED:
/* We are only interested in state-changed messages from the pipeline */
if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data.pipeline)) {
  GstState old_state, new_state, pending_state;
  gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
  g_message ("Pipeline state changed from %s to %s:\n",
      gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
}
break;
```

이 코드조각은 bus로부터 상태 변화에 대한 메시지를 받아 화면에 표시하여 상태 변화에 대한 이해를 돕기 위해 삽입되었다.
모든 element는 현재 상태에 대한 메시지를 bus로 전송하므로, pipeline에 대한 메시지만 필터링해서 볼 필요가 있다.

대부분의 어플리케이션은 미디어를 재생하기 위한 `GST_STATE_PLAYING`, 일시정지를 위한 `GST_STATE_PAUSED`,
그리고 프로그램을 종료하고 모든 리소스를 정리하는 `GST_STATE_NULL` 상태만 관심있게 신경 쓰면 된다.

# Exercise

이번에는 한 번 비디오의 경우에도 구현해 보라.

## Result


# Conclusion

* 이벤트에 대한 알림은 `GstSignals`을 통해 받을 수 있다.
* 부모 element를 통하지 않고 `GstPad`를 직접 연결할 수 있다.
* GStreamer에 대한 다양한 상태들


# References
* Gstreamer official tutorials - [Basic tutorial 3: Dynamic pipelines](https://gstreamer.freedesktop.org/documentation/tutorials/basic/dynamic-pipelines.html?gi-language=c)
