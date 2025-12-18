import React, { useCallback, useEffect } from 'react'

const TARGET_FPS = 75
const TARGET_FRAME_TIME = 1000 / TARGET_FPS

export default function useRAFLoop(fn: Function) {
  const requestRef: any = React.useRef(null)
  const previousTimeRef: any = React.useRef(null)
  const lastFrameTimeRef: any = React.useRef(null)

  const animate = useCallback(
    (time: number) => {
      if (lastFrameTimeRef.current === null) {
        lastFrameTimeRef.current = time
      }

      const elapsed = time - lastFrameTimeRef.current
      if (elapsed >= TARGET_FRAME_TIME) {
        if (previousTimeRef.current !== undefined) {
          const deltaTime = time - previousTimeRef.current
          if (deltaTime >= TARGET_FRAME_TIME) {
            fn(deltaTime)
          }
        }
        previousTimeRef.current = time
        lastFrameTimeRef.current = time
      }
      requestRef.current = requestAnimationFrame(animate)
    },
    [fn],
  )

  useEffect(() => {
    requestRef.current = requestAnimationFrame(animate)
    return () => cancelAnimationFrame(requestRef.current)
  }, [animate]) // Make sure the effect runs only once
}
