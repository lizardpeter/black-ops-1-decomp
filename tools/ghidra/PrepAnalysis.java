// Pre-analysis script for every build: log the JVM heap actually available, and run the analyzers that
// have exhausted the heap on these binaries with a single thread.
//
// CoDMPServer_s and the retail t5mp/t5sp ran out of heap inside the x86 constant-propagation analyzer
// within minutes; after that the program database is unusable for every later step.
//@category T5
import java.util.Map;

import ghidra.app.script.GhidraScript;

public class PrepAnalysis extends GhidraScript {
	@Override
	public void run() throws Exception {
		Runtime rt = Runtime.getRuntime();
		println(String.format("JVM max heap: %d MB, processors: %d", rt.maxMemory() >> 20, rt.availableProcessors()));

		Map<String, String> options = getCurrentAnalysisOptionsAndValues(currentProgram);
		for (Map.Entry<String, String> e : options.entrySet()) {
			String name = e.getKey();
			if (name.contains("Constant Reference Analyzer") || name.contains("Decompiler Switch Analysis")
					|| name.contains("Stack")) {
				println("option: " + name + " = " + e.getValue());
			}
			if (name.endsWith(".Max Threads") && (name.contains("Constant Reference Analyzer")
					|| name.contains("Decompiler Switch Analysis") || name.contains("Decompiler Parameter ID"))) {
				setAnalysisOption(currentProgram, name, "1");
				println("set " + name + " = 1");
			}
		}
	}
}
