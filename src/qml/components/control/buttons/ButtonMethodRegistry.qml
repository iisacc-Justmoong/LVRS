import QtQml

QtObject {
    id: registry

    property var owner: null
    property string defaultTrigger: "manual"
    property var method: null
    property var methods: []

    readonly property bool hasInjectedMethods: collectMethods().length > 0

    function appendMethod(result, candidate) {
        if (candidate === undefined || candidate === null)
            return
        if (typeof candidate === "function"
                || typeof candidate.invoke === "function"
                || typeof candidate.trigger === "function") {
            result.push(candidate)
            return
        }
        if (Array.isArray(candidate)) {
            for (let i = 0; i < candidate.length; i++)
                appendMethod(result, candidate[i])
        }
    }

    function collectMethods() {
        const result = []
        appendMethod(result, method)
        appendMethod(result, methods)
        return result
    }

    function createEvent(triggerName) {
        const source = owner
        const triggerValue = triggerName === undefined || triggerName === null || String(triggerName).length === 0
            ? defaultTrigger
            : String(triggerName)
        return {
            "source": source,
            "trigger": triggerValue,
            "enabled": source && source.enabled !== undefined ? source.enabled : true,
            "effectiveEnabled": source && source.effectiveEnabled !== undefined
                ? source.effectiveEnabled
                : source && source.enabled !== undefined
                    ? source.enabled
                    : true,
            "tone": source && source.tone !== undefined ? source.tone : undefined
        }
    }

    function invokeMethod(candidate, eventData) {
        if (candidate === undefined || candidate === null)
            return undefined
        if (typeof candidate === "function")
            return candidate(eventData)
        if (typeof candidate.invoke === "function")
            return candidate.invoke(eventData)
        if (typeof candidate.trigger === "function")
            return candidate.trigger(eventData)

        console.warn("LVRS ButtonMethodRegistry ignored a non-callable injected method.")
        return undefined
    }

    function invokeMethods(eventData) {
        const eventPayload = eventData === undefined || eventData === null
            ? createEvent(defaultTrigger)
            : eventData
        const injectedMethods = collectMethods()
        const results = []
        for (let i = 0; i < injectedMethods.length; i++)
            results.push(invokeMethod(injectedMethods[i], eventPayload))
        return results
    }
}

// API usage (internal):
// ButtonMethodRegistry { owner: control; method: function(eventData) { ... } }
