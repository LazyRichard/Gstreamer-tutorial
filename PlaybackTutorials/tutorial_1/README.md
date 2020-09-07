# Playback tutorial 1: Playbin usage

## Goal

* 파일에 얼마나 많은 스트림이 있는지 확인하고 이를 스위칭하는 법
* 각 스트림에 따른 정보를 얻는 법

## Introduction

여러 채널의 오디오나 비디오, 자막 스트림이 하나에 임베드 되어 있는 파일을 볼 수 있다. 가장 자주 볼 수 있는 케이스는 일반적인 영화 파일로 하나의 영상과 하나의 음성 스트림을 갖는다(스테레오 또는 5.1 오디오 트랙은 하나의 스트림으로 간주한다.). 또 늘어나고 있는 일반적인 케이스는 여러 언어를 지원하기 위해 하나의 비디오에 여러 음성 스트림을 갖는 영화 파일이다. 이러한 케이스에서는 사용자가 하나의 오디오 스트림을 선택하면 어플리케이션은 선택된 것을 재생하고 나머지는 무시한다.

적합한 스트림을 선택하기 위해서는 사용자는 스트림에 대한 정보를 알아야 한다. 예를 들자면 언어가 될 수 있다. 이러한 정보는 메타데이터(annexed 데이터)라는 형태로 스트림에 임베드 되어 있다. 그리고 이 예제에서 어떻게 가져올 수 있는지 알아볼 예정이다.



## The multilingual player

```
1 video stream(s), 3 audio stream(s), 0 text stream(s)

video stream (NULL):
  codec: On2 VP8    

audio stream 0:     
  codec: Vorbis     
  language: pt
  bitrate: 160000
audio stream 1:
  codec: Vorbis
  language: en
  bitrate: 160000
audio stream 2:
  codec: Vorbis
  language: es
  bitrate: 160000


** Message: 10:51:57.713: Currently playing video stream 0, audio stream 0 and text stream -1
Type any number and hit ENTER to select a different audio stream
1
** Message: 10:52:06.709: Setting current audio stream to 1

2
** Message: 10:52:09.315: Setting current audio stream to 2
```

## Walkthrough

```c
/* Structure to contain all our information, so we can pass it around */
typedef struct _CustomData {
  GstElement *playbin; // Out one and only element

  gint n_video; // Number of embedded video streams
  gint n_audio; // Number of embedded audio streams
  gint n_text;  // Number of embedded subtitle streams

  gint current_video; // Currently playing video stream
  gint current_audio; // Currently playing audio stream
  gint current_text;  // Currently playing subtitle stream

  GMainLoop *main_loop; // Glib's Main Loop
} CustomData;
```

평소처럼 구조체에 사용할 모든 변수를 넣어 함수들에 전달할 수 있도록 한다. 이 예제에서는 각 얼마나 많은 스트림이 있는지와 현재 재생중인 스트림을 저장할 필요가 있다. 또한 상호작용을 위해 메시지를 기다리는 다른 메커니즘을 적용하기 위해 GLib의 메인 루프 객체를 사용한다.

/* playbin flags */
typedef enum {
  GST_PLAY_FLAG_VIDEO = (1 << 0), // We want video output
  GST_PLAY_FLAG_AUDIO = (1 << 1), // We want audio output
  GST_PLAY_FLAG_TEXT = (1 << 2)   // We want subtitle output
} GstPlayFlags;

나중에 사용하기 위해 `playbin`의 플래그를 정의한다. 열거형을 사용해 이러한 플래그를 조작하기 쉽게 사용하고 싶지만 `playbin`은 GStreamer 코어에 들어있지 않은 플러그인이기 때문에 이러한 열거형은 직접 사용할 수 없다. 이를 해결하기 위해 약간의 트릭을 사용해 `playbin` 문서에 정의된 열거형을 그대로 코드에 선언해준다. GObject는 introspection을 허용하기 때문에 이러한 플래그를 트릭을 사용하지 않고 런타임중에 가져올 수 있지만 여기서는 넘어가도록 하겠다.

```c
/* Forward definition for the message and keyboard processing functions */
static gboolean handle_message(GstBus *bus, GstMessage *msg, CustomData *data);
static gboolean handle_keyboard(GIOChannel *source, GIOCondition cond, CustomData *data);
```

콜백으로 사용할 함수즐에 대한 전방 선언(Forward declaration)을 한다. `handle_message`는 GStreamer의 메시지를 처기하기 위한 것이며, 이 예제에서는 사용자와 키보드를 이용해 상호작용하는 부분이 포함되므로 `handle_keyboard`는 키보드 입력을 처리한다.

```c
/* Set flags to show Audio and Video but ignore Subtitles */
g_object_get(data.playbin, "flags", &flags, NULL);
flags |= GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO;
flags &= ~GST_PLAY_FLAG_TEXT;
g_object_set(data.playbin, "flags", flags, NULL);
```

`playbin`의 행동은 `flags` 프로퍼티를 수정함으로써 변경할 수 있다. 그리고 `flags`플래그는 `GstPlayFlags`의 조합을 통해 만들 수 있다. 

Flag                     | Description
---                      |---
GST_PLAY_FLAG_VIDEO      | 비디오 렌더링을 활성화 한다. 만약 플레그가 세트되지 않으면 비디오는 출력되지 않는다.
GST_PLAY_FLAG_AUDIO      | 오디오 렌더링을 활성화 한다. 만약 플레그가 세트되지 않으면 오디오는 출력되지 않는다.
GST_PLAY_FLAG_TEXT       | 자막 렌더링을 활성화 한다. 만약 플레그가 세트되지 않으면 자막은 보이지 않는다.
GST_PLAY_FLAG_VIS        | 비디오 스트림이 없을 때 시각화를 렌더링을 활성화 한다. Playback tutorial 6: Audio visulization에서 자세한 내용을 확인할 수 있다.
GST_PLAY_FLAG_DOWNLOAD   | Basic tutorial 12: Streaming과 Playback tutorial 4: Progressive streaming을 참고하라.
GST_PLAY_FLAG_BUFFERING  | Basic Tutorial 12: Streaming and Playback tutorial 4: Progressive streaming을 참고하라.
GST_PLAY_FLAG_DEINTERACE | 비디오 컨텐츠가 인터레이스이면 이 플레그는 `playbin`이 표시하기 전에 디인터레이스를 수행하도록 한다.

이번 경우에는 시연을 목적으로 하기 때문에 오디오와 비디오는 활성화 하지만 자막은 비활성화 했다. 그리고 그 외의 플레그는 기본값으로 둔다. (그래서 먼저 `g_object_get()`를 통해 현재 플레그 값을 가져와서 수정 한 다음 `g_object_set()`를 통해 플레그를 넣어준다.)

```c
/* Set connection speed. This will affect some internal decisions of playbin */
g_object_set(data.playbin, "connection-speed", 56, NULL);
```

이 프로퍼티는 여기 예제에서는 그다지 유용하지 않다. `connection-speed`는 서버에 여러 버전의 미이어가 요청가능할 경우 `playbin`에게 네트워크 최대 속도를 알려줘 적합한 미디어를 선택할 수 있도록 한다. 이는 스트리밍 프로토콜인 `rtsp`나 `mms`에 주로 사용된다.

프로퍼티를 하나하나 설정할 수도 있지만 한 번에 설정할 수도 있다.

```c
g_object_set (data.playbin, "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_cropped_multilingual.webm", "flags", flags, "connection-speed", 56, NULL);
```

이러한 이유로 `g_object_set()`는 마지막 파라미터로 NULL을 필요로 한다.

```c
/* Add a keyboard watch so we get notified when message arrives */
#ifdef G_OS_WIN32
  io_stdin = g_io_channel_win32_new_fd(_fileno(stdin));
#else
  io_stdin = g_io_channel_unix_new(_fileno(stdin));
#endif
  g_io_add_watch(io_stdin, G_IO_IN, (GIOFunc)handle_keyboard, &data);
```

이 코드조각은 표준 입력(여기서는 키보드)에 대해 콜백함수를 연결한다. 여기서는 GStreamer가 아닌 GLib을 이용하므로 깊게 설명하지는 않겠다. 응용프로그램은 주로 고유한  방법을 통해 사용자 입력을 처리한다. GStreamer는 Tutorial 17: DVD playback에서 간략하게 논의한 탐색 인터페이스 외에는 거의 관련이 없다.

```c
/* Create a GLib Main Loop and set it to run */
data.main_loop = g_main_loop_new(NULL, FALSE);
g_main_loop_run(data.main_loop);
```

상호작용을 위해 더 이상 GStreamer bus를 폴링하지 않고 `GMainLoop`(GLib main loop)를 만들고 `g_main_loop_run()`을 통해 처리한다. 이 함수 플록은 `g_main_loop_quit()`가 호출되기 전까지는 리턴하지 않는다. 그 동안 미리 등록한 콜백을 실행한다. `handle_message`는 bus가 나타나면 호출된다. 그리고 사용자 입력이 들어오면 `handle_keyboard`가 호출된다.

`handle_message`는 파이프라인이 `GST_STATE_PLAYING` 상태로 변할 때를 제외하고는 특별히 새로울 것은 없다. `GST_STATE_PLAYING` 상태로 변할 때, `analyze_streams`함수를 호출한다.

```c
/* Extract some metadata from the streams and print it the stream */
static void analyze_streams(CustomData *data) {
  GstTagList *tags;
  gchar *str;
  guint rate;

  /* Read some properties */
  g_object_get(data->playbin, "n-video", &data->n_video, NULL);
  g_object_get(data->playbin, "n-audio", &data->n_audio, NULL);
  g_object_get(data->playbin, "n-text", &data->n_text, NULL);
```

주석을 통해 알 수 있듯, 이 함수는 미디어로부터 정보를 획득해 화면에 출력해준다. 비디오, 오디오 그리고 자막 스트림의 갯수를 `n-video`, `n-audio` 그리고 `n-text` 프로퍼티를 통해 접근할 수 있다.

```c
for (gint i = 0; i < data->n_video; i++) {
  tags = NULL;

  /* Retrieve the stream's video tags */
  g_signal_emit_by_name(data->playbin, "get-video-tags", i, &tags);
  if (tags) {
    g_print("video stream %s:\n", i);
    gst_tag_list_get_string(tags, GST_TAG_VIDEO_CODEC, &str);
    g_print("  codec: %s\n", str ? str : "unknown");
    g_free(str);
    gst_tag_list_free(tags);
  }
}
```

이제 각각의 스트림에 대해 메타데이터를 가져온다. 메타데이터는 태그의 형태로 `GstTagList` 구조체에 저장된다. 이는 리스트 형태의 데이터로 고유한 이름을 갖는다. 스트림과 연관된 `GstTagList`는 `g_signal_emit_by_name()`을 통해 가져올 수 있고, 각각 태그는 `gst_tag_list_get_string()`과 같은 `gst_tag_list_get_*` 시리즈의 함수를 이용해 얻을 수 있다.

`playbin`은 3개의 엑션 시그널을 정의해 메타데이터를 가져올 수 있다. `gst-video-tags`, `gst-audio-tags` 그리고 `gst-text-tags`이다. 태그가 표준화 되어 있다면 이러한 이름들은 `GstTagList `문서에서 찾을 수 있다. 예를 들어 스트림의 `GST_TAG_LANGUAGE_CODE`이나 `GST_TAG_*CODEC`(*은 audio, video 또는 text)처럼 말이다.

```c
g_object_get(data->playbin, "current-video", &data->current_video, NULL);
g_object_get(data->playbin, "current-audio", &data->current_audio, NULL);
g_object_get(data->playbin, "current-text", &data->current_text, NULL);
```

필요한 모든 메타데이터를 얻었으면 `playbin`으로 부터 이제 현재 선택된 스트림에 대한 정보를 가져와야 한다. `current-video`, `current-audio` 그리고 `current-text`이다. 

현재 선택된 스트림은 항상 관심있게 확인해야 하고 어떠한 가정도 하지 않는 것이 좋다. 여러가지 내부조건으로 인해 다른 실행 환경에서 `playbin`은 다르게 행동할 수 있다. 또한 스트림이 나열되는 순서가 한 번의 실행에서 다른 실행으로 바뀔때 변경될 수 있으므로 메타데이터를 점검하여 하나의 특정 스트림을 식별하는 것이 중요한다.

```c
/* Process keyboard input */
static gboolean handle_keyboard(GIOChannel *source, GIOCondition cond, CustomData *data) {
  gchar *str = NULL;

  if (g_io_channel_read_line(source, &str, NULL, NULL, NULL) == G_IO_STATUS_NORMAL) {
    guint64 index = g_ascii_strtoull(str, NULL, 0);
    if (index < 0 || index >= data->n_audio) {
      g_error("Index out of bounds\n");
    } else {
      /* If the input was a valid audio stream index, set the current audio stream */
      g_message("Setting current audio stream to %d\n", index);
      g_object_set(data->playbin, "current-audio", index, NULL);
    }
  }
  g_free(str);
  return TRUE;
}
```

마지막으로 사용자로부터 재생되는 오디오 스트림을 변경할 수 있도록 하는 부분이다. 정말 기본적인 함수로 표준 입력으로부터 문자열을 읽어서 숫자로 해석한 다음 `playbin`의 `current-audio` 프로퍼티를 설정한다.

항상 기억해야 할 것은 스위칭은 즉각적이지 않다는 점이다. 이전에 이미 디코드 된 오디오가 pipeline을 통해 흐르고 있고 그 동안 새로운 스트림이 활성화 되어 디코드 될 것이다. 이 딜레이는 멀티플렉싱 된 컨테이나 `playbin`의 내부 큐의 길이 에 따라 달라질 수 있다.

# Conclusion

* `playbin`의 몇 가지 프로퍼티들: `flags`, `connection-speed`, `n-video`, `n-audio`, `n-text`, `current-video`, `current-audio`, `current-text`.
* 어떻게 스트림과 연관된 태그를 `g_signal_emit_by_name()`를 이용해 가져오는지
* 어떻게 리스트로부터 특정 태그를 `gst_tag_list_get_string()`또는 `gst_tag_list_get_uint()`를 통해 가져오는지
* 어떻게 `current-audio` 프로퍼티를 이용해 현재 재생중인 오디오를 바꾸는지

# References

* Gstreamer official tutorials - [Playback tutorial 1: Playbin usage](https://gstreamer.freedesktop.org/documentation/tutorials/playback/playbin-usage.html?gi-language=c)
