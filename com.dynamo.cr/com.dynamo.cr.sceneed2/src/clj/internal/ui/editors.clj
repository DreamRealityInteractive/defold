(ns internal.ui.editors
  (:require [clojure.core.async :refer [put!]]
            [dynamo.project :as p]
            [dynamo.file :as f]
            [internal.system :as sys]
            [camel-snake-kebab :refer :all]
            [service.log :as log])
  (:import  [org.eclipse.swt.widgets Display Listener]
            [org.eclipse.swt SWT]))

(defn display
  []
  (or (Display/getCurrent) (Display/getDefault)))

(defn swt-thread-safe*
  [f]
  (.asyncExec (display) f))

(defmacro swt-safe
  [& body]
  `(swt-thread-safe* (bound-fn [] ~@body)))

(defmacro swt-await
  [& body]
  `(let [res# (promise)]
     (.syncExec (display)
       (bound-fn [] (deliver res# (do ~@body))))
     (deref res#)))

(defn swt-timed-exec*
  [after f]
  (when-let [cur (display)]
    (.timerExec cur after f)))

(defmacro swt-timed-exec
  [after & body]
  `(swt-timed-exec* ~after (bound-fn [] ~@body)))

(defmacro swt-events [& evts]
  (let [keywords (map (comp keyword ->kebab-case str) evts)
        constants (map symbol (map #(str "SWT/" %) evts))]
    (zipmap keywords constants)))

(deftype EventForwarder [ch]
  Listener
  (handleEvent [this evt] (put! ch evt)))

(def event-map (swt-events Dispose Resize Paint MouseDown MouseUp MouseDoubleClick
                           MouseEnter MouseExit MouseHover MouseMove MouseWheel DragDetect
                           FocusIn FocusOut Gesture KeyUp KeyDown MenuDetect Traverse))

(def event-type-map (clojure.set/map-invert event-map))

(defn event-type [^org.eclipse.swt.widgets.Event evt]
  (event-type-map (.type evt)))

(defn listen
  [component type callback-fn]
  (.addListener component (event-map type)
    (proxy [Listener] []
      (handleEvent [evt]
        (callback-fn evt)))))

(defn pipe-events-to-channel
  [control type ch]
  (.addListener control (event-map type)
    (EventForwarder. ch)))

(defn implementation-for
  "Factory for values that implement the Editor protocol.
   When called with an editor site and an input file, returns an
   appropriate Editor value."
  [site file]
  (let [proj (sys/project-state)]
    ((p/editor-for
       proj
       (.. file getFullPath getFileExtension))
      proj site file)))
