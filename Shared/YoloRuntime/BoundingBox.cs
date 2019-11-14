using System.Drawing;

/// <summary>
/// https://docs.microsoft.com/en-us/windows/uwp/winrt-components/creating-windows-runtime-components-in-csharp-and-visual-basic
/// https://docs.microsoft.com/en-us/dotnet/machine-learning/tutorials/object-detection-onnx
/// https://github.com/onnx/models/tree/master/vision/object_detection_segmentation/tiny_yolov2
/// </summary>

namespace YoloRuntime
{
    /// <summary>
    /// Pass bounding box from managed (C#) windows runtime (C++/CX)
    /// </summary>
    public sealed class BoundingBox
    {

        public string Label { get; set; }
        public int TopLabel { get; set; }

        // Also add a get/set to allow for int to be passed
        public float X { get; set; }
        public float Y { get; set; }

        public float Height { get; set; }
        public float Width { get; set; }

        public float Confidence { get; set; }

        public Windows.Foundation.Rect Rect
        {
            get { return new Windows.Foundation.Rect(X, Y, Width, Height); }
        }
    }
}
