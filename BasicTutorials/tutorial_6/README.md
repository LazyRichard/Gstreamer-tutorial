# Basic tutorial 6: Media formats and Pad Capabilities

## Goal

* pad capabilities가 무엇인지
* 어떻게 얻는지
* 언제 얻는지
* 왜 알아야 하는지

## Introduction

### Pads

이전에 이미 보여줬던 것처럼 Pad는 정보가 element에 들어갈 수 있도록 한다. Pad의 Capabilities(줄여서 Caps)는 Pad를 통할 수 있는 정보의 종류를 특정한다. 예를 들면 "초당 30프레임이며 320x200 픽셀의 해상도를 갖는 RGB 비디오", "샘플 오디오당 16비트, 초당 44100 샘플을 갖는 5.1채널" 심지어 또는 mp3나 h264와 같은 압축 포멧을 지정할 수도 있다.

Pad는 여러개의 Capabilities를 지원할 수 있다. 예를 들면 비디오 sink는 RGB또는 YUV 포멧의 영상을 지원한다. 그리고 Capabilities는 범위를 특정할 수도 있다. 예를 들면 오디오 sink는 초당 샘플 레이트를 1에서 48000까지 지원한다. 그러나 실제 Pad에서 Pad로 통하는 정보는 오직 하나의 잘 정의된 타입이어야 한다. *협상(Negotiation)*이라는 과정을 통해 연결된 Pad는 사용할 수 있는 Capabilities를 범위가 아닌 하나의 타입으로 고정 시킨다.

두 개의 element가 서로 연결되려면 공통된 Capabilities를 가져야 한다. 그렇지 않으면 그들은 서로 이해하지 못할 것이다.

어플리케이션 개발자로써, 보통 element들을 연결해서 pipeline을 만든다. (혹은 `playbin`과 같이 기 정의 된 pipeline을 확장하던지). 이 경우 element의 Pad Caps를 알아야 한다. 적어도 GStreamer가 두 element를 연결할 때 왜 협상 과정에서 실패하는지는 알아야 한다.

### Pad templates

Pad는 Pad가 갖을 수 있는 모든 Capabilities에 대해 정의되어 있는 Pad 템플릿을 통해 만들어진다. 템플릿은 비슷한 Pad를 만들 때 유용하며, element 사이의 연결을 조기에 종료시킬 수 있다. 만약 Pad의 Capabilities 템플릿이 공통된 서브셋을 갖지 않는다면 더이상의 협상은 무의미하기 때문이다.

Pad 템플릿은 협상 과정 중 첫 단계로 볼 수 있다. 프로세스가 진행됨에 따라 실제 Pad가 인스턴스화 되고 Capabilities가 고정되거나 협상이 실패할 때까지 다듬어 진다.

### Capabilities examples

```
SINK template: 'sink'
  Availability: Always
  Capabilities:
    audio/x-raw
               format: S16LE
                 rate: [ 1, 2147483647 ]
             channels: [ 1, 2 ]
    audio/x-raw
               format: U8
                 rate: [ 1, 2147483647 ]
             channels: [ 1, 2 ]
```

이 pad는 모든 element에서 사용할 수 있는 sink이다(여기서는 가용성에 대한 이야기는 하지 않을 예정이다.). 이는 정수 포멧의 두 가지 raw 오디오 미디어를 지원한다. 하나는 16 비트의 리틀 엔디언이고 하나는 unsigned 8 비트이다. 대괄호안에 있는 값은 범위를 나타낸다. 예를 들면 여기서 채널은 1부터 2를 지원한다.

```
SRC template: 'src'
  Availability: Always
  Capabilities:
    video/x-raw
                width: [ 1, 2147483647 ]
               height: [ 1, 2147483647 ]
            framerate: [ 0/1, 2147483647/1 ]
               format: { I420, NV12, NV21, YV12, YUY2, Y42B, Y444, YUV9, YVU9, Y41B, Y800, Y8, GREY, Y16 , UYVY, YVYU, IYU1, v308, AYUV, A420 }
```

`video/x-raw`는 이 source pad가 raw 비디오를 출력하는 것을 나타낸다. 다양한 크기와 프레임레이트를 지원하며 중괄호({})는 리스트를 나타내는데 그 안에서 확인할 수 있듯 YUV 포멧을 지원한다.

## Last remarks

`gst-inspect-1.0` 도구를 사용해 GStreamer의 element에 대해 Caps를 확인할 수 있다.

주의해야 할 점은 몇몇 element 들은 일반적으로 `GST_STATE_READY` 상태 이상으로 진입할 때 기본 하드웨어에 지원되는 형식을 조회하고 그에 따라 Pad Caps를 제공할 수도 있다. 따라서 표시된 Caps는 플랫폼마다도 다를 수 있지만 심지어 드물지만 한 실행에서 다음 실행 때 달라질 수도 있다.

이번 예제에서는 팩토리를 이용해 두 element를 생성하고 그들의 Pad 템플릿을 확인하며, pipeline에서 재생될 수 있도록 연결 합니다. 각각 상태가 변할 때, sink element의 Pad Capabilities가 어떻게 변하는지 확인한다. 따라서 Pad Caps가 어떻게 협상과정을 통해 고정되는지 확인할 수 있다.

## A trival Pad Capabilities Example

```
Pad Templates for Audio test source:
  SRC template: 'src'
    Availability: Always
    Capabilities:
     audio/x-raw
               format: { (string)S16LE, (string)S16BE, (string)U16LE, (string)U16BE, (string)S24_32LE, (string)S24_32BE, (string)U24_32LE, (string)U24_32BE, (string)S32LE, (string)S32BE, (string)U32LE, (string)U32BE, (string)S24LE, (string)S24BE, (string)U24LE, (string)U24BE, (string)S20LE, (string)S20BE, (string)U20LE, (string)U20BE, (string)S18LE, (string)S18BE, (string)U18LE, (string)U18BE, (string)F32LE, (string)F32BE, (string)F64LE, (string)F64BE, (string)S8, (string)U8 }
               layout: { (string)interleaved, (string)non-interleaved }
                 rate: [ 1, 2147483647 ]
             channels: [ 1, 2147483647 ]

Pad Templates for Auto audio sink:
   SINK template: 'sink'
    Availability: Always
    Capabilities:
     ANY

** Message: 03:29:02.659: In NULL state:

Caps for the sink pad:
      ANY
** Message: 03:29:02.675: Pipeline state changed from NULL to READY:
Caps for the sink pad:
      audio/x-raw
                format: F32LE
                layout: interleaved
                  rate: 48000
              channels: 2
          channel-mask: 0x0000000000000003
** Message: 03:29:02.698: Pipeline state changed from READY to PAUSED:
Caps for the sink pad:
      audio/x-raw
                format: F32LE
                layout: interleaved
                  rate: 48000
              channels: 2
          channel-mask: 0x0000000000000003
** Message: 03:29:02.701: Pipeline state changed from PAUSED to PLAYING:
Caps for the sink pad:
      audio/x-raw
                format: F32LE
                layout: interleaved
                  rate: 48000
              channels: 2
          channel-mask: 0x0000000000000003
```
## Walkthrough

```c
static void print_pad_capabilities(GstElement *element, gchar *pad_name) {
  GstPad *pad = NULL;
  GstCaps *caps = NULL;

  /* Retrieve pad */
  pad = gst_element_get_static_pad(element, pad_name);
  if (!pad) {
    g_error("Could not retrieve pad '%s'", pad_name);

    return;
  }

  /* Retrieve negotiated caps (or acceptable caps if negotiation if not finished
   * yet) */
  caps = gst_pad_get_current_caps(pad);
  if (!caps) {
    caps = gst_pad_query_caps(pad, NULL);
  }

  /* Print and free */
  g_print("Caps for the %s pad:\n", pad_name);
  print_caps(caps, "      ");
  gst_caps_unref(caps);
  gst_object_unref(pad);
}
```

`gst_element_get_static_pad()`는 주어진 element로 부터 이름을 통해 Pad를 가져온다. 이 Pad는 element에서 항상 보여지므로 *static*이다. Pad의 가용성에 대해서는 `Gstreamer documentation`을 참고하라.

그 다음 `gst_pad_get_current_caps()` 함수를 이용해 Pad의 현재 Capabilities(협상 과정에 따라 고정되었을 수도 있고 아닐 수도 있다.)를 가져온다. 현재 수용할 수 있는 Caps는 `GST_STATE_NULL`상태일 때는 Pad의 템플릿에 정의된 Caps지만 다른 상태일 때는 변경될 수도 있고 실제 물리적인 하드웨어에 따라 조회될 수도 있다.

```c
/* Create the element factories */
source_factory = gst_element_factory_find("audiotestsrc");
sink_factory = gst_element_factory_find("autoaudiosink");
if (!source_factory || !sink_factory) {
  g_error("Not all element factories could be created.\n");
  return -1;
}

/* Print information about the pad templates of these factories */
print_pad_templates_information(source_factory);
print_pad_templates_information(sink_factory);

/* Ask the factories to instantiate actual elements */
source = gst_element_factory_create(source_factory, "source");
sink = gst_element_factory_create(sink_factory, "sink");
```

이전 튜토리얼에서는 `gst_element_factory_make()`를 통해 element를 생성하고 팩토리에 대해서는 언급을 하지 않고 넘어갔지만 이번에 할 차례이다. `GstElementFactory`는 팩토리 이름에 따라 주어진 element에 대해서 인스턴스 만드는 것을 관장한다.

`gst_element_factory_find()`를 이용해 "videotestsrc"라는 타입을 갖는 팩토리를 찾고 이를 `gst_element_factory_create()` 함수를 통해 인스턴스화 한다. `gst_element_factory_make()`는 `gst_element_factory_find()`와 `gst_element_factory_create()` 과정을 단축시켜주는 숏컷이다.

Pad 템플릿은 팩토리를 통해 이미 엑세스가 가능하므로 팩토리가 생성된 직후에 출력할 수 있다.

기존에 했었던 파이프라인 생성, 시작 부분을 건너뛰고 상태 변화 메시지를 처리하는 부분만 설펴보자.

```c
case GST_MESSAGE_STATE_CHANGED:
    /* We are only interested in state-changed messages from the pipeline */
    if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
      GstState old_state, new_state, pending_state;
      gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
      g_message("Pipeline state changed from %s to %s:", gst_element_state_get_name(old_state),
                gst_element_state_get_name(new_state));
      /* Print the current capabilities of the sink element */
      print_pad_capabilities(sink, "sink");
    }
    break;
```

pipeline의 상태가 변할 때마다 현재 Pad Caps를 출력한다. 예제의 결과를 확인하면 알 수 있겠지만, 초기에 Caps이 어떻게 되는지(Pad의 Caps 템플릿)부터 범위가 아닌 하나의 값으로 고정되기까지 점진적인 과정을 알 수 있다.

# Conclusion

* Pad Capabilities와 Pad Capabilities 템플릿이 무엇인지.
* `gst_pad_get_current_caps()` 또는 `gst_pad_query_caps()`를 통해 어떻게 가져오는지
* pipeline의 상태에 따라 서로 다른 의미를 갖는 다는 것(처음에는 가능한 모든 Capabilities를 나타내지만 나중에는 협상 중인 Pad의 Caps를 나타냄.)
* Pad Caps는 두 element를 연결하기위해 중요하다는 것
* Pad Caps는 `gst-inspect-1.0`을 통해 확인할 수 있다는 것

# References

* Gstreamer official tutorials - [Basic tutorial 6: Media formats and Pad Capabilities](https://gstreamer.freedesktop.org/documentation/tutorials/basic/time-management.html?gi-language=c)
