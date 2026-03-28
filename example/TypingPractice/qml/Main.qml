import QtQuick
import LVRS 1.0 as LV

LV.ApplicationWindow {
    id: root

    visible: true
    width: 920
    height: 760
    title: "Typing Practice"
    subtitle: "LVRS Example"
    navigationEnabled: false
    readonly property rect exampleViewport: layoutSafeAreaBounds
    property bool viewportContractReady: exampleViewport.width > 0 && exampleViewport.height > 0

    readonly property var practiceTexts: [
        "Framework design drives implementation quality.",
        "Predictable APIs reduce integration risk.",
        "Measure latency, then optimize the critical path.",
        "Small iterative improvements compound over time.",
        "Readable code is cheaper to maintain."
    ]

    property int textIndex: 0
    property string targetText: String(practiceTexts[textIndex])
    property string typedText: ""
    property bool running: false
    property bool roundFinished: false
    property int elapsedSeconds: 0
    property int completedRounds: 0
    property string statusMessage: "Press Start to begin typing practice."

    readonly property int targetLength: targetText.length
    readonly property int typedLength: typedText.length
    readonly property int comparedLength: Math.min(typedLength, targetLength)
    readonly property int correctChars: calculateCorrectChars()
    readonly property int wrongChars: comparedLength - correctChars
    readonly property real progressChars: Math.min(typedLength, targetLength)
    readonly property real progressPercent: targetLength > 0 ? (progressChars / targetLength) * 100.0 : 0
    readonly property real accuracy: typedLength > 0 ? (correctChars / typedLength) * 100.0 : 100.0
    readonly property real elapsedMinutes: elapsedSeconds > 0 ? elapsedSeconds / 60.0 : 0.0
    readonly property real wpm: elapsedMinutes > 0 ? (correctChars / 5.0) / elapsedMinutes : 0.0
    readonly property real cpm: elapsedMinutes > 0 ? typedLength / elapsedMinutes : 0.0

    Timer {
        id: sessionTimer
        interval: 1000
        repeat: true
        running: root.running
        onTriggered: root.elapsedSeconds += 1
    }

    function roundValue(value) {
        return Math.round(value * 10) / 10
    }

    function calculateCorrectChars() {
        const max = Math.min(typedText.length, targetText.length)
        let correct = 0
        for (let i = 0; i < max; i++) {
            if (typedText.charAt(i) === targetText.charAt(i))
                correct += 1
        }
        return correct
    }

    function evaluateInput(text) {
        typedText = text
        if (!running || roundFinished)
            return

        if (typedText === targetText)
            finishRound()
    }

    function startRound() {
        running = true
        roundFinished = false
        elapsedSeconds = 0
        typedText = ""
        typingInput.text = ""
        statusMessage = "In progress. Type the sentence exactly to finish."
        typingInput.forceInputFocus()
    }

    function finishRound() {
        running = false
        roundFinished = true
        completedRounds += 1
        statusMessage = "Completed: Accuracy " + roundValue(accuracy) + "%, WPM " + roundValue(wpm)
    }

    function resetRound() {
        running = false
        roundFinished = false
        elapsedSeconds = 0
        typedText = ""
        typingInput.text = ""
        statusMessage = "Reset complete. Press Start to begin again."
    }

    function nextText() {
        textIndex = (textIndex + 1) % practiceTexts.length
        targetText = String(practiceTexts[textIndex])
        resetRound()
        statusMessage = "Switched to a new sentence."
    }

    Flickable {
        id: scrollArea
        x: root.exampleViewport.x + LV.Theme.gap24
        y: root.exampleViewport.y + LV.Theme.gap24
        width: Math.max(1, root.exampleViewport.width - (LV.Theme.gap24 * 2))
        height: Math.max(1, root.exampleViewport.height - (LV.Theme.gap24 * 2))
        clip: true
        contentWidth: width
        contentHeight: contentColumn.implicitHeight

        Column {
            id: contentColumn
            width: scrollArea.width
            spacing: LV.Theme.gap12

            LV.AppCard {
                id: targetCard
                width: parent.width
                title: "Target Sentence"
                subtitle: "Type the sentence accurately"

                Column {
                    width: targetCard.width - (targetCard.cardPadding * 2)
                    spacing: LV.Theme.gap10

                    LV.Label {
                        width: parent.width
                        style: body
                        wrapMode: Text.WordWrap
                        text: root.targetText
                    }

                    LV.ProgressBar {
                        width: parent.width
                        size: regular
                        startValue: 0
                        endValue: root.targetLength
                        currentValue: root.progressChars
                    }

                    LV.Label {
                        width: parent.width
                        style: caption
                        text: "Progress: " + root.progressChars + " / " + root.targetLength
                    }
                }
            }

            LV.AppCard {
                id: inputCard
                width: parent.width
                title: "Typing Input"
                subtitle: "Input area"

                Column {
                    width: inputCard.width - (inputCard.cardPadding * 2)
                    spacing: LV.Theme.gap10

                    LV.InputField {
                        id: typingInput
                        width: parent.width
                        enabled: root.running && !root.roundFinished
                        placeholderText: "Press Start to enable input"
                        onTextEdited: root.evaluateInput(text)
                        onAccepted: root.evaluateInput(text)
                    }

                    Row {
                        spacing: LV.Theme.gap8

                        LV.LabelButton {
                            text: root.running ? "Running" : "Start"
                            tone: LV.AbstractButton.Primary
                            enabled: !root.running
                            onClicked: root.startRound()
                        }

                        LV.LabelButton {
                            text: "Reset"
                            tone: LV.AbstractButton.Default
                            enabled: !root.running
                            onClicked: root.resetRound()
                        }

                        LV.LabelButton {
                            text: "Next Text"
                            tone: LV.AbstractButton.Default
                            enabled: !root.running
                            onClicked: root.nextText()
                        }
                    }

                    LV.Label {
                        width: parent.width
                        style: description
                        wrapMode: Text.WordWrap
                        text: root.statusMessage
                    }
                }
            }

            LV.AppCard {
                id: metricsCard
                width: parent.width
                title: "Metrics"
                subtitle: "Accuracy and speed metrics"

                Column {
                    width: metricsCard.width - (metricsCard.cardPadding * 2)
                    spacing: LV.Theme.gap8

                    Row {
                        spacing: LV.Theme.gap8
                        LV.Label { style: body; text: "Elapsed: " + root.elapsedSeconds + "s" }
                        LV.Label { style: body; text: "Rounds: " + root.completedRounds }
                    }

                    Row {
                        spacing: LV.Theme.gap8
                        LV.Label { style: body; text: "Correct: " + root.correctChars }
                        LV.Label { style: body; text: "Wrong: " + root.wrongChars }
                    }

                    Row {
                        spacing: LV.Theme.gap8
                        LV.Label { style: body; text: "Accuracy: " + root.roundValue(root.accuracy) + "%" }
                        LV.Label { style: body; text: "WPM: " + root.roundValue(root.wpm) }
                        LV.Label { style: body; text: "CPM: " + root.roundValue(root.cpm) }
                    }

                    LV.Label {
                        width: parent.width
                        style: caption
                        wrapMode: Text.WordWrap
                        text: "Input length: " + root.typedLength + " / " + root.targetLength
                    }
                }
            }
        }
    }
}
