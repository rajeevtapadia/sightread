import { Song, SongMeasure, SongNote } from '@/types'
import { isBrowser } from '@/utils'

export default class FreePlayer {
  time: number = 0
  lastTime: number = 0
  raf: number | undefined
  song: Song
  active: Map<number, number> // Map from midiNote --> time created.
  tempoBpm: number
  timeSignature: { numerator: number; denominator: number }

  constructor() {
    this.time = Number.MAX_SAFE_INTEGER
    this.lastTime = 0
    this.active = new Map()
    this.tempoBpm = 120
    this.timeSignature = { numerator: 4, denominator: 4 }
    this.song = {
      bpms: [],
      tracks: { 1: { instrument: 'piano' } },
      measures: [],
      notes: [],
      duration: 0,
      items: [],
      keySignature: 'C',
      ppq: 480,
      ticksToSeconds: (ticks: number) => ticks / 480 / 2, // Assuming 120 bpm
      secondsToTicks: (seconds: number) => seconds * 480 * 2,
    }
    this.song.items = this.song.notes // Hack – measures will be recomputed on demand
    if (isBrowser()) {
      this.loop()
    }
  }

  start() {
    this.time = Number.MAX_SAFE_INTEGER
    this.lastTime = Date.now()
    this.active.clear()
    this.loop()
  }

  stop() {
    if (typeof this.raf === 'number') {
      cancelAnimationFrame(this.raf)
    }
  }

  loop() {
    this.raf = requestAnimationFrame(() => {
      const now = Date.now()
      const dt = now - this.lastTime
      this.time -= dt
      this.lastTime = now

      // Extend each note.
      for (let [midiNote, pressedTime] of this.active.entries()) {
        let note = this.song.notes.find((n) => n.midiNote === midiNote)
        if (note) {
          note.time = this.getTime()
          note.duration = pressedTime - note.time
        }
      }

      // Synthesize measure items for the falling-notes visualization based on
      // the current time, tempo and time signature. Keep items sorted by time
      // so the visualization utilities (getItemsInView/getRange) behave
      // correctly.
      const secondsVisible = 20
      const currentTime = this.getTime()
      this.song.measures = this.getMeasuresAround(currentTime, secondsVisible)
      this.song.items = [...this.song.measures, ...this.song.notes].sort(
        (a, b) => a.time - b.time,
      )

      this.loop()
    })
  }

  addNote(midiNote: number, velocity: number = 80) {
    const time = this.getTime()
    const note: SongNote = {
      midiNote,
      velocity,
      type: 'note',
      track: 1,
      time,
      duration: 0,
      measure: 0,
    }
    this.song.notes.unshift(note)
    this.active.set(midiNote, time)
  }
  releaseNote(midiNote: number) {
    this.active.delete(midiNote)
  }

  // In seconds
  getTime() {
    return this.time / 1000
  }

  setTempo(tempoBpm: number) {
    this.tempoBpm = tempoBpm
    this.song.bpms = [{ time: 0, bpm: tempoBpm }]
  }

  setTimeSignature(sig: { numerator: number; denominator: number }) {
    this.timeSignature = sig
    this.song.timeSignature = sig
  }

  /**
   * Compute a list of measures around the current viewport based on the
   * configured tempo and time signature. This is only used for Freeplay,
   * where there is no pre-existing score.
   */
  getMeasuresAround(time: number, secondsVisible: number): SongMeasure[] {
    const beatsPerMeasure = this.timeSignature.numerator || 4
    const secondsPerBeat = 60 / (this.tempoBpm || 120)
    const secondsPerMeasure = beatsPerMeasure * secondsPerBeat

    if (!Number.isFinite(secondsPerMeasure) || secondsPerMeasure <= 0) {
      return []
    }

    const halfWindow = secondsVisible / 2
    const startTime = Math.max(0, time - halfWindow)
    const endTime = time + secondsVisible * 1.5

    const firstMeasureIndex = Math.floor(startTime / secondsPerMeasure)
    const lastMeasureIndex = Math.ceil(endTime / secondsPerMeasure)

    const measures: SongMeasure[] = []
    for (let i = firstMeasureIndex; i <= lastMeasureIndex; i++) {
      const measureTime = i * secondsPerMeasure
      measures.push({
        type: 'measure',
        time: measureTime,
        duration: secondsPerMeasure,
        number: i + 1,
      })
    }

    return measures
  }
}
