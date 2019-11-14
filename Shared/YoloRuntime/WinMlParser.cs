using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Linq;

/// <summary>
/// https://docs.microsoft.com/en-us/windows/uwp/winrt-components/creating-windows-runtime-components-in-csharp-and-visual-basic
/// https://docs.microsoft.com/en-us/dotnet/machine-learning/tutorials/object-detection-onnx
/// https://github.com/onnx/models/tree/master/vision/object_detection_segmentation/tiny_yolov2
/// </summary>
/// 
namespace YoloRuntime
{
    /// <summary>
    /// https://docs.microsoft.com/en-us/windows/uwp/winrt-components/creating-windows-runtime-components-in-csharp-and-visual-basic
    /// Pass managed code (C#) to windows runtime (C++/CX)
    /// Require: 
    /// </summary>
    public sealed class WinMlParserInterop
    {
        WinMlParser winMlParser;

        public void InitializeInterop()
        {
            winMlParser = new WinMlParser();
        }

        // Interop method to perform non max
        // suppression using WinMlParser class.
        public IList<BoundingBox>
            NonMaxSuppressInterop(
            IList<BoundingBox> boxesInterop,
            int limit,
            float threshold)
        {
            // Call the regular method to sort boxes
            return winMlParser.NonMaxSuppress(
                    boxesInterop,
                    limit,
                    threshold);
        }

        // Interop method to parse the Yolo outputs
        // using WinMlParser class.
        public IList<BoundingBox> ParseOutputsInterop(
            IReadOnlyList<float> yoloModelOutputs,
            float threshold)
        {
            // Call the regular method to parse outputs
            float[] yoloModelOutputsFloatArr = yoloModelOutputs.ToArray();

            return winMlParser.ParseOutputs(
                yoloModelOutputsFloatArr,
                threshold);
        }
    }

    class WinMlParser
    {
        public const int ROW_COUNT = 13;
        public const int COL_COUNT = 13;
        public const int CHANNEL_COUNT = 125;
        public const int BOXES_PER_CELL = 5;
        public const int BOX_INFO_FEATURE_COUNT = 5;
        public const int CLASS_COUNT = 20;
        public const float CELL_WIDTH = 32;
        public const float CELL_HEIGHT = 32;

        private int channelStride = ROW_COUNT * COL_COUNT;

        private float[] anchors = new float[]
            {
                1.08F, 1.19F, 3.42F, 4.41F, 6.63F, 11.38F, 9.42F, 5.11F, 16.62F, 10.52F
            };

        private string[] labels = new string[]
            {
                "aeroplane", "bicycle", "bird", "boat", "bottle",
                "bus", "car", "cat", "chair", "cow",
                "diningtable", "dog", "horse", "motorbike", "person",
                "pottedplant", "sheep", "sofa", "train", "tvmonitor"
            };

        public IList<BoundingBox> ParseOutputs(
            float[] yoloModelOutputs,
            float threshold)
        {
            var boxes = new List<BoundingBox>();

            var featuresPerBox = BOX_INFO_FEATURE_COUNT + CLASS_COUNT;
            var stride = featuresPerBox * BOXES_PER_CELL;

            for (int cy = 0; cy < ROW_COUNT; cy++)
            {
                for (int cx = 0; cx < COL_COUNT; cx++)
                {
                    for (int b = 0; b < BOXES_PER_CELL; b++)
                    {
                        var channel = (b * (CLASS_COUNT + BOX_INFO_FEATURE_COUNT));

                        var tx = yoloModelOutputs[GetOffset(cx, cy, channel)];
                        var ty = yoloModelOutputs[GetOffset(cx, cy, channel + 1)];
                        var tw = yoloModelOutputs[GetOffset(cx, cy, channel + 2)];
                        var th = yoloModelOutputs[GetOffset(cx, cy, channel + 3)];
                        var tc = yoloModelOutputs[GetOffset(cx, cy, channel + 4)];

                        var x = ((float)cx + Sigmoid(tx)) * CELL_WIDTH;
                        var y = ((float)cy + Sigmoid(ty)) * CELL_HEIGHT;
                        var width = (float)Math.Exp(tw) * CELL_WIDTH * this.anchors[b * 2];
                        var height = (float)Math.Exp(th) * CELL_HEIGHT * this.anchors[b * 2 + 1];

                        var confidence = Sigmoid(tc);

                        if (confidence < threshold)
                            continue;

                        var classes = new float[CLASS_COUNT];
                        var classOffset = channel + BOX_INFO_FEATURE_COUNT;

                        for (int i = 0; i < CLASS_COUNT; i++)
                            classes[i] = yoloModelOutputs[GetOffset(cx, cy, i + classOffset)];

                        var results = Softmax(classes)
                            .Select((v, ik) => new { Value = v, Index = ik });

                        var topClass = results.OrderByDescending(r => r.Value).First().Index;
                        var topScore = results.OrderByDescending(r => r.Value).First().Value * confidence;
                        var testSum = results.Sum(r => r.Value);

                        if (topScore < threshold)
                            continue;

                        boxes.Add(new BoundingBox()
                        {
                            Confidence = topScore * 100.0f,
                            X = (x - width / 2),
                            Y = (y - height / 2),
                            Width = width,
                            Height = height,
                            TopLabel = topClass,
                            Label = this.labels[topClass]
                        });
                    }
                }
            }

            return boxes;
        }

        // Threshold used for IOU computation
        public IList<BoundingBox> NonMaxSuppress(
            IList<BoundingBox> boxes,
            int limit, float threshold)
        {
            var activeCount = boxes.Count;
            var isActiveBoxes = new bool[boxes.Count];

            for (int i = 0; i < isActiveBoxes.Length; i++)
                isActiveBoxes[i] = true;

            var sortedBoxes = boxes.Select((b, i) => new { Box = b, Index = i })
                                .OrderByDescending(b => b.Box.Confidence)
                                .ToList();

            var results = new List<BoundingBox>();

            for (int i = 0; i < boxes.Count; i++)
            {
                if (isActiveBoxes[i])
                {
                    var boxA = sortedBoxes[i].Box;
                    results.Add(boxA);

                    if (results.Count >= limit)
                        break;

                    for (var j = i + 1; j < boxes.Count; j++)
                    {
                        if (isActiveBoxes[j])
                        {
                            var boxB = sortedBoxes[j].Box;

                            if (IntersectionOverUnion(boxA.Rect, boxB.Rect) > threshold)
                            {
                                isActiveBoxes[j] = false;
                                activeCount--;

                                if (activeCount <= 0)
                                    break;
                            }
                        }
                    }

                    if (activeCount <= 0)
                        break;
                }
            }

            return results;
        }

        // IoU (Intersection over union) measures the overlap between 2 boundaries. We use that to
        // measure how much our predicted boundary overlaps with the ground truth (the real object
        // boundary). In some datasets, we predefine an IoU threshold (say 0.5) in classifying
        // whether the prediction is a true positive or a false positive. This method filters
        // overlapping bounding boxes with lower probabilities.
        private float IntersectionOverUnion(
            Windows.Foundation.Rect a,
            Windows.Foundation.Rect b)
        {
            var areaA = (float)a.Width * (float)a.Height;
            var areaB = (float)b.Width * (float)b.Height;

            if (areaA <= 0 || areaB <= 0)
                return 0;

            var minX = Math.Max((float)a.Left, (float)b.Left);
            var minY = Math.Max((float)a.Top, (float)b.Top);
            var maxX = Math.Min((float)a.Right, (float)b.Right);
            var maxY = Math.Min((float)a.Bottom, (float)b.Bottom);

            float intersectionArea = Math.Max(maxY - minY, 0) * Math.Max(maxX - minX, 0);

            return intersectionArea / (areaA + areaB - intersectionArea);
        }

        private int GetOffset(int x, int y, int channel)
        {
            // YOLO outputs a tensor that has a shape of 125x13x13, which 
            // WinML flattens into a 1D array.  To access a specific channel 
            // for a given (x,y) cell position, we need to calculate an offset
            // into the array
            return (channel * this.channelStride) + (y * COL_COUNT) + x;
        }

        private float Sigmoid(float value)
        {
            var k = (float)Math.Exp(value);

            return k / (1.0f + k);
        }

        private float[] Softmax(float[] values)
        {
            var maxVal = values.Max();
            var exp = values.Select(v => Math.Exp(v - maxVal));
            var sumExp = exp.Sum();

            return exp.Select(v => (float)(v / sumExp)).ToArray();
        }
    }

}
