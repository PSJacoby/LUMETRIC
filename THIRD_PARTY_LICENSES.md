# Third Party Licenses

This project uses the following third-party libraries. Please review their license terms below.

---

## Java Standard Library (java.*, javax.*)

- **License:** GNU General Public License v2 with Classpath Exception (OpenJDK) or Oracle Binary Code License (Oracle JDK)
- **Summary:**
  - You may use, modify, and distribute code that uses the Java standard library without affecting your own code's license, due to the Classpath Exception.
  - See: <https://openjdk.org/legal/gplv2+ce.html>

---

## JFreeChart

- **Imports:**
  - org.jfree.chart.ChartFactory
  - org.jfree.chart.ChartPanel
  - org.jfree.chart.JFreeChart
  - org.jfree.chart.plot.PlotOrientation
  - org.jfree.chart.plot.XYPlot
  - org.jfree.chart.plot.ValueMarker
  - org.jfree.chart.renderer.xy.*
  - org.jfree.data.xy.XYSeries
  - org.jfree.data.xy.XYSeriesCollection
- **License:** GNU Lesser General Public License v2.1 (LGPL v2.1)
- **Summary:**
  - You may use JFreeChart in your project, but you must allow users to replace or update the JFreeChart library (e.g., by using it as a separate JAR file).
  - You must provide a copy of the LGPL license and state your use of JFreeChart.
  - If you modify JFreeChart itself, you must release those modifications under LGPL v2.1.
  - See: <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html>

---

## ImageJ

- **Imports:**
  - ij.IJ
  - ij.ImagePlus
  - ij.process.ImageProcessor
  - ij.process.ByteProcessor
  - ij.process.ShortProcessor
  - ij.gui.ImageCanvas
  - ij.gui.Overlay
  - ij.gui.Roi
- **License:** Public Domain (core ImageJ)
- **Summary:**
  - You may use, modify, and distribute ImageJ code freely. Some plugins may have other licenses; check their documentation if used.
  - See: <https://imagej.nih.gov/ij/docs/faqs.html#license>
