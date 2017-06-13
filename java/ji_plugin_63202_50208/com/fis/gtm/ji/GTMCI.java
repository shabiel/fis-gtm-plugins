/****************************************************************
*								*
*	Copyright 2013 Fidelity Information Services, Inc	*
*								*
*	This source code contains the intellectual property	*
*	of its copyright holder(s), and is made available	*
*	under a license.  If you do not know the terms of	*
*	the license, please stop and do not read further.	*
*								*
****************************************************************/
package com.fis.gtm.ji;

import java.lang.Thread.UncaughtExceptionHandler;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;

public class GTMCI {
	private static native void doVoidCallin(String routine, Object[] args);
	private static native boolean doBooleanCallin(String routine, Object[] args);
	private static native int doIntCallin(String routine, Object[] args);
	private static native long doLongCallin(String routine, Object[] args);
	private static native float doFloatCallin(String routine, Object[] args);
	private static native double doDoubleCallin(String routine, Object[] args);
	private static native String doStringCallin(String routine, Object[] args);
	private static native byte[] doByteArrayCallin(String routine, Object[] args);

	/* Invokes gtm_init(), which forces GT.M to record the ID of the main thread. */
	private static native boolean doInit();
	/* Invokes gtm_exit(). */
	private static native void doShutdown();
	/* Executor service that ensures that all requests are handled in one thread. */
	private static ExecutorService gtmThread = Executors.newSingleThreadExecutor(
		new ThreadFactory() {
			@Override
			public Thread newThread(Runnable runnable) {
				/* Since isInitDone is already set, we are requesting a second thread, which would have a different
				 * ID and would therefore violate our design. So, just exit. */
				if (isInitDone && !isInCallout()) {
					System.err.println("GTM-E-JNI, The main GT.M thread has terminated! Exiting...");
					System.exit(-1);
				}
				Thread thread = Executors.defaultThreadFactory().newThread(runnable);
				/* The main GT.M thread is a daemon thread, so that the JVM would not wait on it while trying to
				 * terminate. */
				thread.setDaemon(true);
				thread.setUncaughtExceptionHandler(new UncaughtExceptionHandler() {
					@Override
					public void uncaughtException(Thread thread, Throwable exception) {
						/* We should never end up here, but if we do, report the error and exit. */
						System.err.println("GTM-E-JNI, Uncaught exception in the main GT.M thread:\n");
						exception.printStackTrace();
						System.exit(-1);
					}
				});
				return thread;
			}
		}
	);

	/* Indicates whether the initialization phase has been completed. */
	private static boolean isInitDone = false;

	static {
		System.loadLibrary("gtmj2m");
		Runtime.getRuntime().addShutdownHook(new Thread() {
			@Override
			public void run() {
				if (!gtmThread.isShutdown())
					gtmThread.shutdownNow();

				/* This system property is set if we do call-outs first; and in this case we do not allow
				 * gtm_exit() to be called. */
				if (isInitDone && !isInCallout())
					doShutdown();
			}
		});
	}

	/*
	 * The following two functions, isInCallout() and checkIfCalloutAndInit(), help us verify whether the originating process
	 * was Java and we are doing a simple call-in:
	 *
	 * 	 --------           --------
	 * 	|  Java  | ======> |  GT.M  |
	 *       --------           --------
	 *
	 * or it was GT.M and we are dealing with a call-in embedded in a call-out:
	 *
	 * 	 --------     1     --------
	 * 	|        | <------ |        |
	 * 	|  Java  |    2    |  GT.M  |
	 * 	|        | ======> |        |
	 *       --------           --------
	 *
	 *  In the second scenario it is the 'gtm.callouts' setting that we define at the launch of the JVM that enables us to
	 *  detect a call-out context (as done in isInCallout()).
	 *
	 *  Note that a Java program can spawn multiple threads, and it is the job of the ExecutorService to guarantee that only
	 *  one and only thread, called GT.M thread, can ever invoke the native functions. Such is the case of simple call-ins. In
	 *  case of a call-in embedded in a call-outs, the GT.M thread is the one doing the call-out, so we cannot allow any other,
	 *  newly spawned threads to do call-ins, which we ensure in ci_gateway.c; for that reason, synchronization is no longer
	 *  required, and the native methods can be called directly (refer to do<Type>Job() methods).
	 */

	/* Indicates whether we are executing in a call-out context. */
	private static boolean isInCallout() {
		return System.getProperty("gtm.callouts") != null;
	}

	/* Checks whether we are inside a call-out, and if so, performs the initialization (in case it has not
	 * been done previously). */
	private static synchronized boolean checkIfCalloutAndInit() {
		boolean isInCallout = isInCallout();
		if (isInCallout && !isInitDone)
			initGTM();
		return isInCallout;
	}

	/* Template of a job which the ExecutorService submits to the GT.M thread. */
	private static final class CallinRequestJob implements Callable<CallinRequest> {
		CallinRequest callinRequest;

		CallinRequestJob(int type, String routineName, Object[] args) {
			this.callinRequest = new CallinRequest(type, routineName, args);
		}

		public CallinRequest call() {
			callinRequest.execute();
			return callinRequest;
		}
	}

	public static void doVoidJob(String name, Object... args) throws Exception {
		if (checkIfCalloutAndInit()) {
			doVoidCallin(name, args);
			return;
		}
		CallinRequest request = gtmThread.submit(new CallinRequestJob(GTMContainerType.GTM_VOID, name, args)).get();
		if (request.exception != null)
			throw request.exception;
	}

	public static boolean doBooleanJob(String name, Object... args) throws Exception {
		if (checkIfCalloutAndInit())
			return doBooleanCallin(name, args);
		CallinRequest request = gtmThread.submit(new CallinRequestJob(GTMContainerType.GTM_BOOLEAN, name, args)).get();
		if (request.exception != null)
			throw request.exception;
		return request.booleanReturnValue;
	}

	public static int doIntJob(String name, Object... args) throws Exception {
		if (checkIfCalloutAndInit())
			return doIntCallin(name, args);
		CallinRequest request = gtmThread.submit(new CallinRequestJob(GTMContainerType.GTM_INTEGER, name, args)).get();
		if (request.exception != null)
			throw request.exception;
		return request.intReturnValue;
	}

	public static long doLongJob(String name, Object... args) throws Exception {
		if (checkIfCalloutAndInit())
			return doLongCallin(name, args);
		CallinRequest request = gtmThread.submit(new CallinRequestJob(GTMContainerType.GTM_LONG, name, args)).get();
		if (request.exception != null)
			throw request.exception;
		return request.longReturnValue;
	}

	public static float doFloatJob(String name, Object... args) throws Exception {
		if (checkIfCalloutAndInit())
			return doFloatCallin(name, args);
		CallinRequest request = gtmThread.submit(new CallinRequestJob(GTMContainerType.GTM_FLOAT, name, args)).get();
		if (request.exception != null)
			throw request.exception;
		return request.floatReturnValue;
	}

	public static double doDoubleJob(String name, Object... args) throws Exception {
		if (checkIfCalloutAndInit())
			return doDoubleCallin(name, args);
		CallinRequest request = gtmThread.submit(new CallinRequestJob(GTMContainerType.GTM_DOUBLE, name, args)).get();
		if (request.exception != null)
			throw request.exception;
		return request.doubleReturnValue;
	}

	public static String doStringJob(String name, Object... args) throws Exception {
		if (checkIfCalloutAndInit())
			return doStringCallin(name, args);
		CallinRequest request = gtmThread.submit(new CallinRequestJob(GTMContainerType.GTM_STRING, name, args)).get();
		if (request.exception != null)
			throw request.exception;
		return request.stringReturnValue;
	}

	public static byte[] doByteArrayJob(String name, Object... args) throws Exception {
		if (checkIfCalloutAndInit())
			return doByteArrayCallin(name, args);
		CallinRequest request = gtmThread.submit(new CallinRequestJob(GTMContainerType.GTM_BYTE_ARRAY, name, args)).get();
		if (request.exception != null)
			throw request.exception;
		return request.byteArrayReturnValue;
	}

	/* Do the call-ins initialization, noting the main GT.M thread. */
	private static void initGTM() {
		if (doInit())
			isInitDone = true;
		else {
			System.err.println("GTM-E-JNI, Cannot start main GT.M thread! Exiting...");
			System.exit(-1);
		}
	}

	/* Holds the details of a call-in request. */
	private static class CallinRequest {
		int type;
		Object[] args;
		String routineName;

		boolean booleanReturnValue;
		int intReturnValue;
		long longReturnValue;
		float floatReturnValue;
		double doubleReturnValue;
		String stringReturnValue;
		byte[] byteArrayReturnValue;

		Exception exception;

		CallinRequest(int type, String routineName, Object[] args) {
			this.type = type;
			this.routineName = routineName;
			this.args = args;
		}

		public void execute() {
			if (!isInitDone)
				initGTM();

			try {
				switch (type) {
				case GTMContainerType.GTM_VOID:
					doVoidCallin(routineName, args);
					break;
				case GTMContainerType.GTM_BOOLEAN:
					booleanReturnValue = doBooleanCallin(routineName, args);
					break;
				case GTMContainerType.GTM_INTEGER:
					intReturnValue = doIntCallin(routineName, args);
					break;
				case GTMContainerType.GTM_LONG:
					longReturnValue = doLongCallin(routineName, args);
					break;
				case GTMContainerType.GTM_FLOAT:
					floatReturnValue = doFloatCallin(routineName, args);
					break;
				case GTMContainerType.GTM_DOUBLE:
					doubleReturnValue = doDoubleCallin(routineName, args);
					break;
				case GTMContainerType.GTM_STRING:
					stringReturnValue = doStringCallin(routineName, args);
					break;
				case GTMContainerType.GTM_BYTE_ARRAY:
					byteArrayReturnValue = doByteArrayCallin(routineName, args);
					break;
				}
			} catch (Exception e) {
				exception = e;
			}
		}
	}
}
