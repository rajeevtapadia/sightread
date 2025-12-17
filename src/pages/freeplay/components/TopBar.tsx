import { Select, TextInput } from '@/components'
import { gmInstruments, InstrumentName } from '@/features/synth'
import { ArrowLeft, Midi, StartRecord, StopRecord } from '@/icons'
import { ButtonWithTooltip } from '@/pages/play/components/TopBar'
import { formatInstrumentName } from '@/utils'
import React, { MouseEvent } from 'react'
import { Link } from 'react-router'

type TopBarProps = {
  isError: boolean
  isLoading: boolean
  isRecordingAudio: boolean
  value: InstrumentName
  onChange: (instrument: InstrumentName) => void
  onClickMidi: (e: MouseEvent<any>) => void
  onClickRecord: (e: MouseEvent<any>) => void
  tempo: number
  onChangeTempo: (tempo: number) => void
  timeSignature: { numerator: number; denominator: number }
  onChangeTimeSignature: (sig: { numerator: number; denominator: number }) => void
}

export default function TopBar({
  isError,
  isLoading,
  isRecordingAudio,
  value,
  onChange,
  onClickMidi,
  onClickRecord,
  tempo,
  onChangeTempo,
  timeSignature,
  onChangeTimeSignature,
}: TopBarProps) {
  const recordTooltip = isRecordingAudio ? 'Stop recording' : 'Start recording audio'

  return (
    <div className="flex h-[50px] min-h-[50px] w-full items-center gap-4 bg-[#292929] px-4 text-2xl text-white transition">
      <ButtonWithTooltip tooltip="Back">
        <Link to="/">
          <ArrowLeft size={24} />
        </Link>
      </ButtonWithTooltip>
      <div className="ml-auto flex items-center gap-3 text-sm">
        <div className="flex items-center gap-2">
          <span className="text-xs text-gray-300">Tempo</span>
          <TextInput
            className="w-16 text-sm text-black"
            type="number"
            min={20}
            max={300}
            value={tempo}
            onChange={(e: React.ChangeEvent<HTMLInputElement>) => {
              const next = Number(e.target.value || 0)
              if (Number.isFinite(next)) {
                const clamped = Math.min(300, Math.max(20, next))
                onChangeTempo(clamped)
              }
            }}
          />
        </div>
        <div className="flex items-center gap-2">
          <span className="text-xs text-gray-300">Time</span>
          <Select
            className="h-3/4 max-w-fit text-xs text-black"
            loading={false}
            error={false}
            value={`${timeSignature.numerator}/${timeSignature.denominator}`}
            onChange={(value: string) => {
              const [numStr, denStr] = value.split('/')
              const numerator = parseInt(numStr, 10) || 4
              const denominator = parseInt(denStr, 10) || 4
              onChangeTimeSignature({ numerator, denominator })
            }}
            options={['2/4', '3/4', '4/4', '6/8', '9/8'] as any}
            format={(v: string) => v}
            display={(v: string) => v}
          />
        </div>
      </div>
      <ButtonWithTooltip
        tooltip={recordTooltip}
        onClick={(e: MouseEvent<any>) => {
          onClickRecord(e)
        }}
      >
        {isRecordingAudio ? <StopRecord size={24} /> : <StartRecord size={24} />}
      </ButtonWithTooltip>
      <ButtonWithTooltip tooltip="Choose a MIDI device" onClick={onClickMidi}>
        <Midi size={24} />
      </ButtonWithTooltip>
      <Select
        className="h-3/4 max-w-fit text-base text-black"
        loading={isLoading}
        error={isError}
        value={value}
        onChange={onChange}
        options={gmInstruments as any}
        format={formatInstrumentName as any}
        display={formatInstrumentName as any}
      />
    </div>
  )
}
