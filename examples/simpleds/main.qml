import QtQuick 2.1
import QtQuick.Controls 2.0
ApplicationWindow {
    id: root
    visible: true
    width: 1024
    height: 640
    title: qsTr("twainTest")
    color: "#1E1E1E"
    modality: Qt.NonModal

    //存储全局变量。
    property QtObject global: QtObject{
        property var camerList: [];
    }
    Row{
        anchors.fill: parent
        anchors.margins: 10
        spacing: 5
        Rectangle {
            height: parent.height
            width: parent.width * 0.18
            radius: 5
            color: "#2D2D2D"
            anchors.margins: 10
            Column{
                width: parent.width * 0.9
                spacing: 10
                anchors.horizontalCenter: parent.horizontalCenter
                PenComboBox {
                    id: cameraList
                    height: 30
                    width: parent.width
                    fontSize: 12
                    textColor: "#9BB3C7"
                    backgoundColor: "#30323D"
                    backgoundRadius: 5
                    model: global.camerList
                    onDisplayTextChanged: {
                        if(displayText.length>1){
                            camersever.selectCamera(displayText);
                        }
                    }
                }
                Button{
                    height: 30
                    width: parent.width
                    background: Rectangle{
                        anchors.fill: parent
                        color: "#67C23A"
                        radius: 5
                        Label {
                            anchors.centerIn: parent
                            text: "截图"
                            color: "white"
                        }
                    }
                    onClicked: {
                        camersever.capture();
                    }
                }
                Component.onCompleted: {
                    camersever.getCameraList();
                }
            }
        }
        Image {
            id: carmeraview
            height: parent.height
            width: parent.width * 0.82 - 5
            cache:false;
        }
    }
    Connections{
        target: camersever
        onImageOutput:{
            carmeraview.source = ""
            carmeraview.source = "image://CodeImg/" + Date.now();
        }
        onSendCameraList:{
            cameraList.model = data;
        }
    }
}
