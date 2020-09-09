# Basic tutorial 9: Media information gathering

## Goal

때때로 미디어 파일(또는 URI)가 어떤 종류인지 빠르게 알아보고 싶거나 재생이 가능한지 여부를 알고 싶을 때가 있다. pipeline을 만들고 재생을 하고 bus의 메시지를 볼 수도 있지만 GStreamer에서는 이러한 목적으로 간단한 유틸리티를 제공한다.

* URI로부터 데이터를 확보하는 방법
* URI가 재생가능한지 여부를 확인하는 방법

## Introduction

`GstDiscoverer`는 `pbutils` 라이브러리(Plug-in Base utilities)에서 찾을 수 있는 유틸리티 객체로 URI또는 URI의 목록을 받아 이들의 정보를 반환한다. 그리고 이 객체는 동기 모드와 비동기 모드로 동작할 수 있다.

동기 모드에서는 오직 정보가 준비될 때까지 블럭하는 `gst_discoverer_discover_uri()` 함수만 실행한다. 블럭 함수인 관계로 GUI 기반 어플리케이션에서는 조금 관심이 떨어진다.

확보된 정보는 코덱에 대한 설명과, 스트림 토폴로지(스트림과 서브 스트림의 갯수) 그리고 사용가능한 메타데이터(오디오 언어와 같은)를 포함한다.

아래 예는 `https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm` 에서 데이터를 얻어온 결과이다.

```
Duration: 0:00:52.250000000
Tags:
  video codec: On2 VP8
  language code: en
  container format: Matroska
  application name: ffmpeg2theora-0.24
  encoder: Xiph.Org libVorbis I 20090709
  encoder version: 0
  audio codec: Vorbis
  nominal bitrate: 80000
  bitrate: 80000
Seekable: yes
Stream information:
  container: WebM
    audio: Vorbis
      Tags:
        language code: en
        container format: Matroska
        audio codec: Vorbis
        application name: ffmpeg2theora-0.24
        encoder: Xiph.Org libVorbis I 20090709
        encoder version: 0
        nominal bitrate: 80000
        bitrate: 80000
    video: VP8
      Tags:
        video codec: VP8 video
        container format: Matroska
```

다음 예제는 커멘드 라인으로부터 주어진 URI로부터 정보를 발견하고 이를 출력한다. (만약 URI가 제공되지 않으면 기본값을 사용한다.) 이것은 간소화된 버전의 `gst-discoverer-1.0` 툴로 어플리케이션이 재생은 하지 않고 메타 데이터만 출력한다.

## The GStreamer Discoverer

```
** Message: 16:33:30.534: Discovering 'https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm'
Discovered 'https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm'

Duration: 0:00:52.250000000
Tags:
  video codec: VP8 video
  datetime: 2012-04-11T16:08:01Z        
  container format: Matroska
  language code: en
  title: Audio
  application name: ffmpeg2theora-0.24  
  encoder: Xiph.Org libVorbis I 20090709
  encoder version: 0
  audio codec: Vorbis
  nominal bitrate: 80000
  bitrate: 80000
Seekable: yes

Stream information:
  container: WebM
    audio: Vorbis
      Tags:
        datetime: 2012-04-11T16:08:01Z
        container format: Matroska
        language code: en
        title: Audio
        application name: ffmpeg2theora-0.24
        encoder: Xiph.Org libVorbis I 20090709
        encoder version: 0
        audio codec: Vorbis
        nominal bitrate: 80000
        bitrate: 80000
    video: VP8
      Tags:
        video codec: VP8 video
        datetime: 2012-04-11T16:08:01Z
        container format: Matroska

Finished discovering
```

## Walkthrough

```c
/* Instantiate the Discoverer */
data.discoverer = gst_discoverer_new(5 * GST_SECOND, &err);
if (!data.discoverer) {
  g_error("Error creating discoverer instance: %s", err->message);
  g_clear_error(&err);

  return -1;
}
```

`gst_discoverer_new()`는 새로운 Discoverer 객체를 만든다. 첫 번채 매개변수는 타임아웃으로 나노초 단위라 `GST_SECOND` 매크로를 이용해 단순화 하였다.

```c
/* Connect to the interesting signals */
g_signal_connect(data.discoverer, "discovered", G_CALLBACK(on_discovered_cb), &data);
g_signal_connect(data.discoverer, "finished", G_CALLBACK(on_finished_cb), &data);
```

관심있는 시그널에 콜백을 연결한다. 나중에 콜백 함수들을 다룰 때 자세히 설명하겠다.

```c
/* Start the discoverer process (nothing to do yet) */
gst_discoverer_start(data.discoverer);
```

`gst_discoverer_start()`로 미디어 정보를 가져오는 프로세스를 시작한다. 하지만 아직 URI를 제공하지 않았다.

```c
/* Add a request to process asynchronously the URI passed through the command line */
if (!gst_discoverer_discover_uri_async(data.discoverer, uri)) {
  g_error("Failed to start discovering URI '%s'", uri);
  g_object_unref(data.discoverer);

  return -1;
}
```

`gst_discoverer_discover_uri_async()` 함수는 제공된 URI를 미디어 정보를 가져오기 위해 큐에 넣는다. 이 함수를 이용해 여러 URI를 큐에 담을 수 있다. 미디어 정보 검색 프로세스가 끝나면 지정된 콜백을 실행한다.

```c
/* Create a GLib Main Loop and set it to run, so we can wait for the signals */
data.loop = g_main_loop_new (NULL, FALSE);
g_main_loop_run (data.loop);
```

기존과 마찬가지로 GLib의 메인 루프를 인스턴스화 하고 실행한다. `g_main_loop_quit()`는 `on_finished_cb` 콜백에서 실행한다.

```c
/* Stop the discoverer process */
gst_discoverer_stop (data.discoverer);
```

discoverer가 모든 역할을 마무리 하면 `gst_discoverer_stop()`를 이용해 정지 시키고, `gst_object_unref()`로 메모리를 해제한다.

```c
/* This function is called every time the discoverer has information regarding
 * one of the URIs we provided.*/
static void on_discovered_cb (GstDiscoverer *discoverer, GstDiscovererInfo *info, GError *err, CustomData *data) {
  GstDiscovererResult result;
  const gchar *uri;
  const GstTagList *tags;
  GstDiscovererStreamInfo *sinfo;

  uri = gst_discoverer_info_get_uri (info);
  result = gst_discoverer_info_get_result (info);

  ...
```

이 콜백 함수는 하나의 URI에 대해 discoverer가 작업을 끝내면 호출된다. 그리고 `GstDiscovererInfo` 구조체 형태로 모든 정보를 제공한다.

`gst_discoverer_info_get_uri()` 함수를 이용해 URI에  `gst_discoverer_info_get_result()`를 통해 정보를 가져온다.

```c
switch (result) {
case GST_DISCOVERER_URI_INVALID:
  g_print("Invalid URI '%s'\n", uri);
  break;
case GST_DISCOVERER_ERROR:
  g_print("Discoverer error: %s\n", err->message);
  break;
case GST_DISCOVERER_TIMEOUT:
  g_print("Timeout\n");
  break;
case GST_DISCOVERER_BUSY:
  g_print("Busy\n");
  break;
case GST_DISCOVERER_MISSING_PLUGINS: {
  const GstStructure *s;
  gchar *str;

  s = gst_discoverer_info_get_misc(info);
  str = gst_structure_to_string(s);

  g_print("Missing plugins: %s\n", str);
  g_free(str);
  break;
}
case GST_DISCOVERER_OK:
  g_print("Discovered '%s'\n", uri);
  break;
}
```

`GST_DISCOVERER_OK` 이외에는 몇 가지 문제가 있어 해당 URI는 재생이 불가능함을 의미한다. 열거형의 값이 상당히 명확하므로 이를 참고하면 된다(`GST_DISCOVERER_BUSY`는 동기 모드에서만 발생하기 때문에 이 예제어서는 사용되지 않는다).

에러가 발생되지 않았다면 `gst_discoverer_info_get_duration()`과 같은 `gst_discoverer_info_get_*` 시리즈의 메서드를 이용해 `GstDiscovererInfo`에 구조체에서 결과를 얻을 수 있다.

...

# Conclusion

* 어떻게 URI에서 `GstDiscoverer`를 이용해 데이터를 가져오는지
* `gst_discoverer_info_et_result()`를 이용해 어떻게 URI가 재생가능한지 알 수 있는지

# References

* Gstreamer official tutorials - [Basic tutorial 9: Media information gathering](https://gstreamer.freedesktop.org/documentation/tutorials/basic/media-information-gathering.html?gi-language=c)
