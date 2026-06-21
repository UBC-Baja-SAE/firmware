import QtQuick
import QtQuick.Controls
import QtMultimedia
import Qt.labs.folderlistmodel

Item {
    id: music

    property color tabActiveColor: "#03101e"
    property string customFontName: ""

    property string currentTrackName: "Select a track"

    function formatTime(ms) {
        if (ms <= 0) return "00:00";
        let totalSeconds = Math.floor(ms / 1000);
        let minutes = Math.floor(totalSeconds / 60);
        let seconds = totalSeconds % 60;
        return minutes.toString().padStart(2, '0') + ":" + seconds.toString().padStart(2, '0');
    }

    function playNext() {
        if (folderModel.count > 0) {
            let nextIndex = Math.floor(Math.random() * folderModel.count);

            if (folderModel.count > 1 && nextIndex === songList.currentIndex) {
                nextIndex = (nextIndex + 1) % folderModel.count;
            }

            songList.currentIndex = nextIndex;
            music.currentTrackName = folderModel.get(nextIndex, "fileName");
            mediaPlayer.source = folderModel.get(nextIndex, "fileUrl");
            mediaPlayer.play();
        }
    }

    MediaPlayer {
        id: mediaPlayer
        audioOutput: AudioOutput {
            volume: 1
        }

        onMediaStatusChanged: {
            if (mediaStatus === MediaPlayer.EndOfMedia) {
                music.playNext();
            }
        }
    }

    FolderListModel {
        id: folderModel
        folder: musicFolderUrl
        nameFilters: ["*.mp3", "*.wav", "*.flac", "*.m4a", "*.ogg"]
        showDirs: false
    }

    Row {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -20
        spacing: 30

        // 1. Playlist Sidebar
        Rectangle {
            width: 400
            height: 340
            anchors.verticalCenter: parent.verticalCenter
            radius: 12
            color: Qt.rgba(music.tabActiveColor.r, music.tabActiveColor.g, music.tabActiveColor.b, 0.2)

            ListView {
                id: songList
                anchors.fill: parent
                anchors.margins: 8
                model: folderModel
                clip: true
                spacing: 5

                delegate: Rectangle {
                    width: ListView.view.width
                    height: 45
                    radius: 6
                    color: songList.currentIndex === index
                        ? Qt.rgba(music.tabActiveColor.r, music.tabActiveColor.g, music.tabActiveColor.b, 0.2)
                        : "transparent"

                    Item {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        clip: true

                        Text {
                            id: delegateText
                            text: model.fileName
                            color: "white"
                            font.family: music.customFontName
                            font.pointSize: 14
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.verticalCenterOffset: -3

                            property bool contentFits: implicitWidth <= parent.width

                            x: contentFits ? 0 : animatedX
                            property real animatedX: 0

                            SequentialAnimation on animatedX {
                                running: !delegateText.contentFits
                                loops: Animation.Infinite

                                PauseAnimation { duration: 2000 }
                                NumberAnimation {
                                    to: delegateText.parent.width - delegateText.implicitWidth
                                    duration: Math.max(1000, (delegateText.implicitWidth - delegateText.parent.width) * 30)
                                }
                                PauseAnimation { duration: 2000 }
                                NumberAnimation {
                                    to: 0
                                    duration: Math.max(1000, (delegateText.implicitWidth - delegateText.parent.width) * 30)
                                }
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            songList.currentIndex = index;
                            music.currentTrackName = model.fileName;
                            mediaPlayer.source = model.fileUrl;
                            mediaPlayer.play();
                        }
                    }
                }
            }
        }

        // 2. Main Player Area
        Rectangle {
            id: playerbg
            width: 600
            height: 200
            anchors.verticalCenter: parent.verticalCenter
            radius: 12
            color: Qt.rgba(music.tabActiveColor.r, music.tabActiveColor.g, music.tabActiveColor.b, 0.2)

            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 8
                spacing: 15

                // Track Title Main Ping-Pong Marquee
                Item {
                    width: parent.width
                    height: 40
                    clip: true

                    Text {
                        id: trackTitleText
                        text: music.currentTrackName
                        color: "white"
                        font.family: music.customFontName
                        font.pointSize: 22
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: -4

                        property bool contentFits: implicitWidth <= parent.width

                        x: contentFits ? (parent.width - implicitWidth) / 2 : animatedX
                        property real animatedX: 0

                        SequentialAnimation on animatedX {
                            running: !trackTitleText.contentFits
                            loops: Animation.Infinite

                            PauseAnimation { duration: 2500 }
                            NumberAnimation {
                                to: trackTitleText.parent.width - trackTitleText.implicitWidth
                                duration: Math.max(1000, (trackTitleText.implicitWidth - trackTitleText.parent.width) * 30)
                            }
                            PauseAnimation { duration: 2500 }
                            NumberAnimation {
                                to: 0
                                duration: Math.max(1000, (trackTitleText.implicitWidth - trackTitleText.parent.width) * 30)
                            }
                        }
                    }
                }

                // Playback Slider Container
                Item {
                    // FIXED: Shrunk width by 16px and centered to create an exact 16px margin from the edges
                    width: parent.width - 16
                    anchors.horizontalCenter: parent.horizontalCenter
                    height: 40

                    Text {
                        id: currentTimeText
                        text: music.formatTime(mediaPlayer.position)
                        color: "white"
                        font.family: music.customFontName
                        font.pointSize: 12
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: -2
                    }

                    Slider {
                        id: progressSlider
                        z: 1 // FIXED: Forces the Nyan Cat GIF to render ON TOP of the text elements

                        anchors.left: currentTimeText.right
                        anchors.right: durationText.left
                        anchors.leftMargin: 15
                        anchors.rightMargin: 15
                        anchors.verticalCenter: parent.verticalCenter

                        height: 20
                        padding: 0

                        from: 0
                        to: mediaPlayer.duration
                        value: mediaPlayer.position
                        onMoved: mediaPlayer.position = value

                        property var rainbowColors: ["#FF0000", "#ffc400", "#FFFF00", "#33FF00", "#00f7ff", "#9c33ff"]
                        property int trackHeight: 12
                        property real gifWidth: 80
                        property real gifHeight: 50

                        background: Rectangle {
                            x: progressSlider.leftPadding
                            y: progressSlider.availableHeight / 2 - height / 2
                            implicitWidth: 200
                            implicitHeight: progressSlider.trackHeight
                            width: progressSlider.availableWidth
                            height: implicitHeight
                            radius: 0
                            color: Qt.rgba(music.tabActiveColor.r, music.tabActiveColor.g, music.tabActiveColor.b, 0.2)

                            Item {
                                width: progressSlider.visualPosition * parent.width
                                height: parent.height
                                clip: true

                                Column {
                                    anchors.fill: parent
                                    Repeater {
                                        model: progressSlider.rainbowColors
                                        Rectangle {
                                            width: progressSlider.availableWidth
                                            height: progressSlider.trackHeight / progressSlider.rainbowColors.length
                                            color: modelData
                                        }
                                    }
                                }
                            }
                        }

                        handle: AnimatedImage {
                            x: progressSlider.leftPadding + (progressSlider.visualPosition * progressSlider.availableWidth) - (width / 2)
                            y: progressSlider.availableHeight / 2 - height / 2
                            width: progressSlider.gifWidth
                            height: progressSlider.gifHeight

                            source: "assets/images/original.gif"
                            fillMode: Image.PreserveAspectFit
                            playing: mediaPlayer.playbackState === MediaPlayer.PlayingState

                            opacity: progressSlider.value > 0 ? 1.0 : 0.0
                            Behavior on opacity { NumberAnimation { duration: 200 } }
                        }
                    }

                    Text {
                        id: durationText
                        text: music.formatTime(mediaPlayer.duration)
                        color: "white"
                        font.family: music.customFontName
                        font.pointSize: 12
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: -2
                    }
                }

                // Playback Controls
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 40

                    Rectangle {
                        width: 50
                        height: 50
                        radius: 8
                        color: mediaPlayer.playbackState === MediaPlayer.PlayingState
                            ? Qt.rgba(music.tabActiveColor.r, music.tabActiveColor.g, music.tabActiveColor.b, 0.2)
                            : "transparent"

                        Image {
                            anchors.centerIn: parent
                            width: 35
                            height: 35
                            source: mediaPlayer.playbackState === MediaPlayer.PlayingState
                                ? "assets/icons/pause.svg"
                                : "assets/icons/play.svg"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (!mediaPlayer.hasAudio && folderModel.count > 0) {
                                    music.playNext();
                                } else if (mediaPlayer.hasAudio) {
                                    if (mediaPlayer.playbackState === MediaPlayer.PlayingState) {
                                        mediaPlayer.pause();
                                    } else {
                                        mediaPlayer.play();
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        width: 50
                        height: 50
                        radius: 8
                        color: skipMouseArea.pressed
                            ? Qt.rgba(music.tabActiveColor.r, music.tabActiveColor.g, music.tabActiveColor.b, 0.2)
                            : "transparent"

                        Image {
                            anchors.centerIn: parent
                            width: 35
                            height: 35
                            source: "assets/icons/skip.svg"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }

                        MouseArea {
                            id: skipMouseArea
                            anchors.fill: parent
                            onClicked: {
                                music.playNext();
                            }
                        }
                    }
                }
            }
        }
    }
}